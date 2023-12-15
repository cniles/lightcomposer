#include <catch2/catch_test_macros.hpp>
#include "SDL_thread.h"
#include "netlights.h"
#include "listener.h"
#include <iostream>

extern "C" {
  #include "SDL.h"
}

TEST_CASE( "publishes multicast", "[netlights]" ) {
  SDL_cond *cond = SDL_CreateCond();

  SDL_Thread *listen_thread = listen_for_traffic(cond);

  SDL_mutex *mutex = SDL_CreateMutex();

  SDL_LockMutex(mutex);
  SDL_CondWait(cond, mutex);
  SDL_UnlockMutex(mutex);

  std::cout << "Waiting for thread to finish" << std::endl;

  int status;

  SDL_DestroyCond(cond);
  SDL_DestroyMutex(mutex);
  SDL_WaitThread(listen_thread, &status);
}

TEST_CASE( "netlights_init clears the queue", "[netlights]" ) {
  BlockingQueue<Uint32*> bc;
  netlights_ctx ctx;

  ctx.lightsq = &bc;
  bc.push(new Uint32(1));
  bc.push(new Uint32(2));
  bc.push(new Uint32(3));
  bc.close();

  netlights_init(&ctx);

  while (!bc.empty()) {
    SDL_Delay(1);
  }

  REQUIRE(bc.is_closed());
}
