#include <catch2/catch_test_macros.hpp>
#include "netlights.h"
#include <iostream>

extern "C" {
  #include "SDL.h"
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
