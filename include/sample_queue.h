#ifndef SAMPLE_QUEUE_H_
#define SAMPLE_QUEUE_H_

#include <list>

extern "C" {
    #include <SDL.h>
}

struct SampleQueue {
  SDL_mutex *mutex;
  SDL_cond *cond;
  std::list<Uint8*> list;
};

void sample_queue_init(SampleQueue &q);

void sample_queue_push(SampleQueue &q, Uint8 *samples);

Uint8 *sample_queue_pop(SampleQueue& q);

void sample_queue_close(SampleQueue &q);

bool sample_queue_empty(SampleQueue &q);

#endif