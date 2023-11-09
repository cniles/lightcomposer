#include <assert.h>
#include <iostream>

#include "mstime.h"
#include "audio.h"
#include "packet_queue.h"
#include "blocking_queue.h"

#ifdef __cplusplus
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
}
#endif

static PacketQueue audioq;
static BlockingQueue<Uint8*> sampleq;
static sample_callback callback;
static SwrContext *swr;

#define SDL_AUDIO_BUFFER_SIZE 1024

#ifdef FMT_16
#define AV_FORMAT AV_SAMPLE_FMT_S16
#define SDL_FORMAT AUDIO_S16
#endif

#ifdef FMT_FLT
#define AV_FORMAT AV_SAMPLE_FMT_FLT
#define SDL_FORMAT AUDIO_F32
#endif

AVCodecContext *get_decoder_for_stream(AVStream *stream) {
  AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);

  if (codec == NULL) {
    std::cout << "Unsupported codec!" << std::endl;
    return NULL;
  }

  AVCodecContext *ctx = avcodec_alloc_context3(codec);
  avcodec_parameters_to_context(ctx, stream->codecpar);
  ctx->request_sample_fmt = AV_FORMAT;
  avcodec_open2(ctx, codec, NULL);

  return ctx;
}

int audio_decode_frame(AVCodecContext *codecCtx, uint8_t *audio_buf,
                       int buf_size) {
  static AVPacket pkt;
  static AVFrame frame;

  int len1, data_size = 0;

  for (;;) {
    if (avcodec_receive_frame(codecCtx, &frame) == 0) {
      data_size = av_samples_get_buffer_size(
          NULL, codecCtx->channels, frame.nb_samples, codecCtx->sample_fmt, 1);
      assert(data_size <= buf_size);
      memcpy(audio_buf, frame.data[0], data_size);
      return data_size;
    }
    if (pkt.data)
      av_packet_unref(&pkt);

    if (packet_queue_get(&audioq, &pkt, 1) < 0) {
      return -1;
    }

    int send_result = avcodec_send_packet(codecCtx, &pkt);
    if (send_result < 0 && send_result != AVERROR(EAGAIN))
      return -1;
  }
}

/**
 * Converts the provided data to FLT format using libswresample
 */
float *resample_to_flt(uint8_t **stream_start, int len,
                       AVSampleFormat in_format, int channels,
                       int *nb_samples) {

  int bytes_per_sample = av_get_bytes_per_sample(in_format);

  *nb_samples = swr_get_out_samples(swr, (len / channels) / bytes_per_sample);

  if (len % bytes_per_sample) {
    std::cout << "Sample data not aligned to input format!" << std::endl;
  }

  uint8_t *output;

  av_samples_alloc(&output, NULL, 1, *nb_samples, AV_SAMPLE_FMT_FLT, 0);

  int out_samples = swr_convert(swr, &output, *nb_samples,
                                (const uint8_t **)stream_start, *nb_samples);

  return (float *)output;
}

int decode_stream(void *userdata, Uint8 *stream, int len) {
  AVCodecContext *codecCtx = (AVCodecContext *)userdata;
  int len1, audio_size;

  Uint8 *streamStart = stream;
  int desiredLen = len;

  static uint8_t audio_buf[(192000 * 3) / 2];
  static unsigned int audio_buf_size = 0;
  static unsigned int audio_buf_index = 0; // the next index to write to

  while (len > 0) {
    if (audio_buf_index >= audio_buf_size) {
      audio_size = audio_decode_frame(codecCtx, audio_buf, sizeof(audio_buf));
      if (audio_size < 0) {
        audio_buf_size = SDL_AUDIO_BUFFER_SIZE;
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

  if (len != 0)
    std::cerr << "We're not getting an aligned sample set for the fft" << std::endl;

  return audio_size < 0;
}

static int DecodeThread(void *userdata) {
  AVCodecContext *codecCtx = (AVCodecContext *)userdata;
  bool done = false;

  while (!done) {
    Uint8 *stream = new Uint8[2048];

    done = decode_stream(userdata, stream, 2048) && packet_queue_empty(&audioq) && audioq.closed;

    sampleq.push(stream);

    int nb_samples;
    float *output = resample_to_flt(&stream, 2048, codecCtx->sample_fmt, codecCtx->channels, &nb_samples);

    callback(output, nb_samples);

    av_freep(&output);
  }

  std::cout << "Done decoding" << std::endl;

  return 0;
}

struct audio_callback_context {
  long start_time;
  Uint8 started = 0;
};

void audio_callback(void *userdata, Uint8 *stream, int len) {
  audio_callback_context *ctx= (audio_callback_context*)userdata;

  if (ctx->started == 0) {
    long delay = ctx->start_time - millis_since_epoch();

    if (delay > 0) {
      SDL_Delay(delay);
    }

    ctx->started = 1;
  }
 
  Uint8 *samples = sampleq.pop();
  
  memcpy(stream, samples, len);
  delete samples;
}

void pr_sdl_audio_spec(SDL_AudioSpec spec) {
  const char* type;
  if (SDL_AUDIO_ISFLOAT(spec.format)) {
    type = "float";
  } else {
    type = "int";
  }
  std::cout << "Format: " << type << std::endl;  
}


SDL_AudioSpec setup_sdl_audio(AVCodecContext *codecCtx, long start_time) {
  SDL_AudioSpec desiredSpec;
  SDL_AudioSpec actualSpec;

  audio_callback_context *ctx = new audio_callback_context();
  ctx->start_time = start_time;
  ctx->started = 0;

  desiredSpec.freq = codecCtx->sample_rate;
  desiredSpec.format = SDL_FORMAT;
  desiredSpec.channels = codecCtx->channels;
  desiredSpec.silence = 0;
  desiredSpec.samples = SDL_AUDIO_BUFFER_SIZE;
  desiredSpec.callback = audio_callback;
  desiredSpec.userdata = (void*)ctx;
  
  std::cout << "Opening SDL audio" << std::endl;
  if (SDL_OpenAudio(&desiredSpec, &actualSpec) < 0) {
    std::cout << "SDL_OpenAudio: " << SDL_GetError() << std::endl;
    abort();
  }

  return actualSpec;
}

int get_stream_idx(AVFormatContext *s) {
  for (int i = 0; i < s->nb_streams; ++i) {
    AVMediaType media_type = s->streams[i]->codecpar->codec_type;
    if (media_type != AVMEDIA_TYPE_UNKNOWN) {
      std::cout << "Codec string: " << media_type << " : " << av_get_media_type_string(media_type) << std::endl;
    } else {
      std::cout << "Unknown codec" << std::endl;
    }
    
    if (s->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
      return i;
    }
  }
  return -1;
}

static int LoadThread(void *ptr) {
  load_context *ld_ctx = (load_context *)ptr;
  AVPacket packet;
  int idx = get_stream_idx(ld_ctx->fmt_ctx);
  while (av_read_frame(ld_ctx->fmt_ctx, &packet) >= 0) {
    if (packet.stream_index == idx) {
      packet_queue_put(&audioq, &packet);
    } else {
      av_packet_unref(&packet);
    }
  }
  *(ld_ctx->packet_queue_loaded) = true;
  delete ld_ctx;
  packet_queue_close(&audioq);
  std::cout << "All packets added to queue" << std::endl;
  return 0;
}

void setup_sw_resampling(AVCodecContext *codecCtx) {
  swr = swr_alloc_set_opts(NULL, AV_CH_LAYOUT_MONO, AV_SAMPLE_FMT_FLT, 44100,
                           codecCtx->channel_layout, codecCtx->sample_fmt,
                           codecCtx->sample_rate, 0, NULL);
  swr_init(swr);
  std::cout << "Initialized sw resampling" << std::endl;
}

int audio_queue_empty() {
  return sampleq.empty() && packet_queue_empty(&audioq);
}

int audio_play_source(const char *url, sample_callback callbackFunc, bool *packet_queue_loaded, long start_time) {
  callback = callbackFunc;
  SDL_AudioSpec actual_spec;
  AVFormatContext *s = NULL;

  if (avformat_open_input(&s, url, NULL, NULL) != 0) {
    std::cout << "avformat_open_input failed" << std::endl;
    return -1;
  }

  if (avformat_find_stream_info(s, NULL) < 0) {
    std::cout << "avformat_find_stream_info failed" << std::endl;
    return -1;
  }

  av_dump_format(s, 0, url, 0);

  int streamIdx = get_stream_idx(s);

  if (streamIdx == -1) {
    std::cout << "Could not find audio stream index" << std::endl;
    return -1;
  }

  AVCodecContext *codecCtx = get_decoder_for_stream(s->streams[streamIdx]);

  actual_spec = setup_sdl_audio(codecCtx, start_time);

  pr_sdl_audio_spec(actual_spec);

  setup_sw_resampling(codecCtx);

  std::cout << "Converting to: " << codecCtx->channels << " channel "
            << av_get_sample_fmt_name(codecCtx->sample_fmt) << std::endl;

  packet_queue_init(&audioq);

  SDL_PauseAudio(0);
  std::cout << "Playing audio" << std::endl;

  load_context *ld_ctx = new load_context();

  ld_ctx->packet_queue_loaded = packet_queue_loaded;
  ld_ctx->fmt_ctx = s;

  SDL_CreateThread(LoadThread, "AudioFileLoadThread", (void *)ld_ctx);
  SDL_CreateThread(DecodeThread, "DecodeAudioThread", (void *)codecCtx);

  return 0;
}
