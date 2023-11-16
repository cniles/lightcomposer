#include <iostream>
#include "blocking_queue.h"
#include "netlights.h"

extern "C" {
  #include <SDL.h>
  #include <SDL_thread.h>
}

int BroadcastThread(void *userdata) {
  netlights_ctx *ctx = (netlights_ctx*)userdata;
  std::cout << "Broadcast thread" << std::endl;

  Uint32 *lights;
  while (ctx->lightsq->pop(&lights) >= 0) {
    delete lights;
  }

  return 0;
}

void netlights_init(netlights_ctx *ctx) {
  SDL_DetachThread(SDL_CreateThread(BroadcastThread, "BroadcastThread", (void*)ctx));
}
