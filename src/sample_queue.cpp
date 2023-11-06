#include "sample_queue.h"

void sample_queue_init(SampleQueue &q) {
  q.mutex = SDL_CreateMutex();
  q.cond = SDL_CreateCond();
}

void sample_queue_push(SampleQueue &q, Uint8 *samples) {
  SDL_LockMutex(q.mutex);

  q.list.push_back(samples);

  SDL_UnlockMutex(q.mutex);
}

Uint8 *sample_queue_pop(SampleQueue &q) {
  SDL_LockMutex(q.mutex);

  if (q.list.empty()) {
    SDL_CondWait(q.cond, q.mutex);
  }

  Uint8 *result = *(q.list.begin());

  q.list.erase(q.list.begin());
  
  SDL_UnlockMutex(q.mutex);

  return result;
}

void sample_queue_close(SampleQueue &q) {
  SDL_DestroyCond(q.cond);
  SDL_DestroyMutex(q.mutex);
}

bool sample_queue_empty(SampleQueue &q) {
  SDL_LockMutex(q.mutex);
  bool empty = q.list.empty();
  SDL_UnlockMutex(q.mutex);
  return empty;
}