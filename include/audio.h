#ifndef AUDIO_H
#define AUDIO_H

#ifdef __cplusplus
extern "C" {
#include <SDL.h>
#include <libavformat/avformat.h>
}
#endif

typedef void (*sample_callback)(float *, int);

struct load_context {
  AVFormatContext *fmt_ctx;
  bool *packet_queue_loaded;
};

int audio_queue_empty();

int audio_play_source(const char *url, sample_callback callback, bool *packet_queue_loaded);

#endif
