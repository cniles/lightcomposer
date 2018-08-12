#ifndef AUDIO_H
#define AUDIO_H

#ifdef __cplusplus
extern "C"
{
    #include <SDL/SDL.h>
}
#endif

typedef void (*sample_callback)(Uint8*,int,int);

int audio_play_source(const char *url, int *interrupt, sample_callback callback);

#endif