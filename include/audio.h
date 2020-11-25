#ifndef AUDIO_H
#define AUDIO_H

#include <libavformat/avformat.h>

#ifdef __cplusplus
extern "C" {
#include <SDL.h>
}
#endif

typedef void (*sample_callback)(float *, int);

struct load_context {
  AVFormatContext *fmt_ctx;
  bool *packet_queue_loaded;
};

int audio_play_source(const char *url, int *interrupt,
                      sample_callback callback,
		      bool *packet_queue_loaded);

#endif
