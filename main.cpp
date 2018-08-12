#include <iostream>
#include <assert.h>

// Need this for CPlusPlus development with this C library
#ifdef __cplusplus
extern "C"
{
    #include <libavformat/avformat.h>
    #include <libavutil/avutil.h>
    #include <libavcodec/avcodec.h>
    #include <SDL/SDL.h>
    #include <SDL/SDL_gfxPrimitives.h>
}
#endif

#include <unistd.h>



struct PacketQueue {
    AVPacketList *first_pkt, *last_pkt;
    int nb_packets;
    int size;
    SDL_mutex *mutex;
    SDL_cond *cond;
};

struct Pixel {
   int value;
   Pixel* next;
};

struct PixelQueue {
    Pixel *first_pixel, *last_pixel;
    int size;
    SDL_mutex *mutex;
    SDL_cond *cond;
};

PacketQueue audioq;
PixelQueue drawq;

void packet_queue_init(PacketQueue *q) {
    memset(q, 0, sizeof(PacketQueue));
    q->mutex = SDL_CreateMutex();
    q->cond = SDL_CreateCond();
}

void pixel_queue_init(PixelQueue* q) {
    memset(q, 0, sizeof(PacketQueue));
    q->mutex = SDL_CreateMutex();
    q->cond = SDL_CreateCond();
}

int pixel_queue_get(PixelQueue * q, Pixel pixels[], int size) {
    int ret = 0;
    SDL_LockMutex(q->mutex);
    int dropped = 0;
    while (dropped < size && q->first_pixel != NULL) {
        Pixel *pixel = &pixels[dropped];
        if (q->first_pixel != NULL) {
            Pixel *pixel0 = q->first_pixel;
            *pixel = *pixel0;
            q->first_pixel = pixel->next;
            if (pixel->next == NULL) {
                q->last_pixel = NULL;
            }
            q->size--;
            dropped++;
            delete pixel0;
            ret = 1;
        }
    }
    SDL_UnlockMutex(q->mutex);

    return ret;
}

int pixel_queue_put(PixelQueue* q, int val) {
    SDL_LockMutex(q->mutex);
    
    Pixel *pixel0 = new Pixel();

    pixel0->next = NULL;
    pixel0->value = val;

    if (q->last_pixel == NULL) {
        q->first_pixel = pixel0;
    } else {
        q->last_pixel->next = pixel0;
    }
    q->last_pixel = pixel0;
    q->size++;
    SDL_UnlockMutex(q->mutex);
    return 0;
}

int packet_queue_put(PacketQueue *q, AVPacket *pkt) {
    AVPacketList *pkt1;
    if (av_dup_packet(pkt) < 0) {
        return -1;
    }
    pkt1 = (AVPacketList*)av_malloc(sizeof(AVPacketList));
    if (!pkt1) {
        return -1;
    }
    pkt1->pkt = *pkt;
    pkt1->next = NULL;

    SDL_LockMutex(q->mutex);

    if (!q->last_pkt)
        q->first_pkt = pkt1;
    else
        q->last_pkt->next = pkt1;
    q->last_pkt = pkt1;
    q->nb_packets++;
    q->size += pkt1->pkt.size;
    SDL_CondSignal(q->cond);

    SDL_UnlockMutex(q->mutex);
    return 0;    
}

int quit = 0;

static int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block) {
    AVPacketList *pkt1;
    int ret;

    SDL_LockMutex(q->mutex);

    for(;;) {
        if (quit) {
            ret = -1;
            break;
        }
        pkt1 = q->first_pkt;

        if (pkt1) {
            q->first_pkt = pkt1->next;
            if (!q->first_pkt) {
                q->last_pkt = NULL;
            }
            q->nb_packets--;
            q->size -= pkt1->pkt.size;
            *pkt = pkt1->pkt;
            av_free(pkt1);
            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            SDL_CondWait(q->cond, q->mutex);
        }
    }
    SDL_UnlockMutex(q->mutex);
    return ret;
}

AVCodecContext* getDecoderFromStream(AVStream* stream) {

    AVCodecContext* codecCtx = stream->codec;

    codecCtx->request_sample_fmt = AV_SAMPLE_FMT_S16;

    AVCodec* codec = avcodec_find_decoder(codecCtx->codec_id);

    if (codec == NULL) {
        std::cout << "Unsupported codec!" << std::endl;
        return NULL;
    }

    AVCodecContext* codecCtxCopy = avcodec_alloc_context3(codec);
    avcodec_copy_context(codecCtxCopy, codecCtx);

    avcodec_open2(codecCtxCopy, codec, NULL);

    return codecCtxCopy;
}

int audio_decode_frame(AVCodecContext* codecCtx, uint8_t* audio_buf, int buf_size) {
    static AVPacket pkt;
  static uint8_t *audio_pkt_data = NULL;
  static int audio_pkt_size = 0;
  static AVFrame frame;

  int len1, data_size = 0;

  for(;;) {
    while(audio_pkt_size > 0) {
        int got_frame = 0;
        len1 = avcodec_decode_audio4(codecCtx, &frame, &got_frame, &pkt);
        if (len1 < 0) {
            /* if error, skip frame */
    	    audio_pkt_size = 0;
    	    break;
        }
        audio_pkt_data += len1;
        audio_pkt_size -= len1;
        data_size = 0;
    
        if(got_frame) {
	        data_size = av_samples_get_buffer_size(NULL, 
                codecCtx->channels,
                frame.nb_samples,
                codecCtx->sample_fmt,
                1);

	        assert(data_size <= buf_size);
	        memcpy(audio_buf, frame.data[0], data_size);
        }
        if(data_size <= 0) {
	        /* No data yet, get more frames */
	        continue;
        }
        /* We have data, return it and come back for more later */
        return data_size;
    }
    if(pkt.data)
      av_free_packet(&pkt);

    if(quit) {
      return -1;
    }

    if(packet_queue_get(&audioq, &pkt, 1) < 0) {
      return -1;
    }
    audio_pkt_data = pkt.data;
    audio_pkt_size = pkt.size;
  }
}

int16_t abs(int16_t a) {
    if (a >= 0) return a;
    return -a;
}

int16_t max(int16_t a, int16_t b) {
    if (a > b) return a;
    return b;
}

void audioCallback(void *userdata, Uint8 *stream, int len) {
    AVCodecContext *codecCtx = (AVCodecContext*)userdata;
    int len1, audio_size;

    Uint8* streamStart = stream;
    int desiredLen = len;

    static uint8_t audio_buf[(192000 * 3) / 2];
    static unsigned int audio_buf_size = 0;
    static unsigned int audio_buf_index = 0; // the next index to write to
    while (len > 0) {
        if (audio_buf_index >= audio_buf_size) {
            audio_size = audio_decode_frame(codecCtx, audio_buf, sizeof(audio_buf));
            if (audio_size < 0) {
                audio_buf_size = 1024;
                memset(audio_buf, 0, audio_buf_size);
            } else {
                audio_buf_size = audio_size;
            }
            audio_buf_index = 0;
        }
        len1 = audio_buf_size - audio_buf_index;
        if (len1 > len)
            len1 = len;
        memcpy(stream, (uint8_t *)audio_buf + audio_buf_index, len1);
        len -= len1;
        stream += len1;
        audio_buf_index += len1;
    }

  
    if (len != 0) std::cerr << "We're not getting an aligned sample set for the fft" << std::endl;

    int pixel_count = 512;

    int pixels[pixel_count];

    memset(pixels, 0, sizeof(pixels));

    // get mono samples assuming 16-bit LRLR..LR order
    for (int i = 0; i < desiredLen + len; i+=4) {
        int16_t l_sample = *(int16_t *)&streamStart[i];
        int16_t r_sample = *(int16_t *)&streamStart[i+2];

        int pixel_idx = ((i / 4) / (512 / pixel_count) % pixel_count);

        pixels[pixel_idx] = max(pixels[pixel_idx], abs((int(l_sample) + r_sample) >> 1));
    }

    for (int i = 0; i < pixel_count; ++i) {
        pixel_queue_put(&drawq, pixels[i]);
    }
}


void setupAudio(AVCodecContext* codecCtx) {
    SDL_AudioSpec desiredSpec;
    SDL_AudioSpec actualSpec;
    desiredSpec.freq = codecCtx->sample_rate;
    desiredSpec.format = AUDIO_S16SYS;
    desiredSpec.channels = codecCtx->channels;
    desiredSpec.silence = 0;
    desiredSpec.samples = 1024;
    desiredSpec.callback = audioCallback;
    desiredSpec.userdata = codecCtx;

    if (SDL_OpenAudio(&desiredSpec, &actualSpec) < 0) {
        std::cout << "SDL_OpenAudio: " << SDL_GetError() << std::endl;
        abort();
    }
}



int main(int, char**)
{
    SDL_Surface* screen = NULL;
    SDL_Surface* gfx = NULL;

    SDL_Event event;
    av_register_all();
    if(SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_VIDEO)) {
        abort();
    }

    screen = SDL_SetVideoMode(640, 480, 32, SDL_SWSURFACE);

    gfx = SDL_CreateRGBSurface(SDL_SWSURFACE, 640, 480,
        screen->format->BitsPerPixel,
        screen->format->Rmask,
        screen->format->Gmask,
        screen->format->Bmask,
        screen->format->Amask);

    AVFormatContext* s = NULL;
    const char* url = "file:stinkfist.flac";
    if (avformat_open_input(&s, url, NULL, NULL) != 0) {
        return -1;        
    }

    if (avformat_find_stream_info(s, NULL) < 0) {
        return -1;
    }

    av_dump_format(s, 0, url, 0);
    int streamIdx = -1;
    
        for (int i = 0; i < s->nb_streams; ++i) {
        if (s->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            streamIdx = i;
            break;
        }
    }

    AVCodecContext* codecCtx = getDecoderFromStream(s->streams[streamIdx]);

    std::cout << "Setting up audio" << std::endl;
    setupAudio(codecCtx);

    std::cout << "Starting audio queue" << std::endl;
    packet_queue_init(&audioq);
    pixel_queue_init(&drawq);

    std::cout << "Starting SDL audio" << std::endl;
    SDL_PauseAudio(0);

    std::cout << "Playing audio" << std::endl;
    AVPacket packet;

    while (av_read_frame(s, &packet)>=0) {
        if (packet.stream_index == streamIdx) {
            packet_queue_put(&audioq, &packet);
        } else {
            av_free_packet(&packet);
        }
        SDL_PollEvent(&event);
    }

    std::cout << "All packets added to queue" << std::endl;

    int offset = 0;

    while (!quit) {

        bool draw = false;
        Pixel pixels[640];
        memset(pixels, 0, sizeof(pixels));

        while(pixel_queue_get(&drawq, pixels, 640)) {
            for (int i = 0; i< 640; ++i) {
                if (pixels[i].value == 0) break;
            float volume = (240.0 * ((float)pixels[i].value / INT16_MAX));

            offset++;
            if (offset >= 640) offset = 0;
            
            lineColor(gfx, offset, 0, offset, 480, 0x000000FF);
            lineColor(gfx, offset, 240, offset, 240+volume, 0xFFFFFFFF);

            }
        }

            SDL_Rect source_left = {0, 0, offset, 480};
            SDL_Rect source_right = {offset, 0, 640-offset, 480};
            SDL_Rect dest_left = {640-offset, 0, offset, 480};
            SDL_Rect dest_right = {0, 0, 640-offset, 480};
            SDL_BlitSurface(gfx, &source_left, screen, &dest_left);
            SDL_BlitSurface(gfx, &source_right, screen, &dest_right);
            SDL_Flip(screen);           


        switch (event.type) {
        case SDL_QUIT:
            quit = 1;
            SDL_Quit();
            exit(0);
        break;
            default:
            break;
        }
    }
}
