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

PacketQueue audioq;

void packet_queue_init(PacketQueue *q) {
    memset(q, 0, sizeof(PacketQueue));
    q->mutex = SDL_CreateMutex();
    q->cond = SDL_CreateCond();
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
      if(len1 < 0) {
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

void audioCallback(void *userdata, Uint8 *stream, int len) {
    AVCodecContext *codecCtx = (AVCodecContext*)userdata;
    int len1, audio_size;

    static uint8_t audio_buf[(192000 * 3) / 2];
    static unsigned int audio_buf_size = 0;
    static unsigned int audio_buf_index = 0;
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
    SDL_Event event;
    av_register_all();
    if(SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        abort();
    }

    AVFormatContext* s = NULL;
    const char* url = "file:Peanuts-Theme.mp3";
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
    
    std::cout << "All packets added to queue" << std::endl;

    sleep(10000);
}
