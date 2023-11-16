#include <catch2/catch_test_macros.hpp>

#include "netlights.h"

TEST_CASE( "netlights_init called with context", "[netlights]" ) {
  BlockingQueue<Uint32*> bc;
  netlights_ctx ctx;
  ctx.lightsq = &bc;  
  netlights_init(&ctx);
}
