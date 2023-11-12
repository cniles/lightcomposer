#include "blocking_queue.h"
#include <iostream>

extern "C" {
  #include <SDL.h>
}

struct broadcast_context {
  int n;
  BlockingQueue<Uint32*> *queue;
};

int BroadcastThread(void *userdata) {
  broadcast_context *ctx = (broadcast_context*)userdata;
  std::cout << "Broadcast thread" << std::endl;

  while (true) {
    Uint32 *lights = ctx->queue->pop();
    std::cout << "Pop lights" << std::endl;
    delete lights;
  }

  delete ctx;
}

void start_netlights(BlockingQueue<Uint32*> *lightsq, int n) {
  std::cout << "Hello netlights" << std::endl;

  broadcast_context *ctx = new broadcast_context();

  ctx->n = n;
  ctx->queue = lightsq;

  SDL_CreateThread(BroadcastThread, "BroadcastThread", (void*)ctx);
}
