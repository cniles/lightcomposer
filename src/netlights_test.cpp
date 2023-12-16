#include <catch2/catch_test_macros.hpp>
#include "SDL_thread.h"
#include "netlights.h"
#include "listener.h"
#include <iostream>

extern "C" {
  #include "SDL.h"
}

TEST_CASE( "publishes multicast", "[netlights]" ) {
  BlockingQueue<Uint32*> bq;
  netlights_ctx ctx;

  ctx.n = 50;
  ctx.lightsq = &bq;

  netlights_init(&ctx);

  SDL_cond *cond = SDL_CreateCond();
  SDL_mutex *mutex = SDL_CreateMutex();

  SDL_Thread *listen_thread = listen_for_traffic(cond);

  SDL_LockMutex(mutex);
  SDL_CondWait(cond, mutex);
  SDL_UnlockMutex(mutex);

  for (int i = 0; i < 50; i++) {
    uint32_t *val = new uint32_t;
    *val = i;
    bq.push(val);
  }

  std::cout << "closing queue" << std::endl;
  bq.close();

  int status;

  SDL_DestroyCond(cond);
  SDL_DestroyMutex(mutex);

  std::cout << "Waiting for listener thread to finish" << std::endl;
  SDL_WaitThread(listen_thread, &status);
}

TEST_CASE( "netlights_init clears the queue", "[netlights]" ) {
  BlockingQueue<uint32_t*> bq;
  netlights_ctx ctx;
  ctx.n = 50;

  ctx.lightsq = &bq;
  bq.push(new Uint32(1));
  bq.push(new Uint32(2));
  bq.push(new Uint32(3));
  bq.close();

  netlights_init(&ctx);

  while (!bq.empty()) {
    SDL_Delay(1);
  }

  REQUIRE(bq.is_closed());
}
