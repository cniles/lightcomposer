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

  while (true) {
    Uint32 *lights = ctx->lightsq->pop();
    std::cout << "Pop lights" << std::endl;
    delete lights;
  }
}

void netlights_init(netlights_ctx *ctx) {
  SDL_Thread *thread = SDL_CreateThread(BroadcastThread, "BroadcastThread", (void*)ctx);

  SDL_WaitThread(thread, NULL);
}
