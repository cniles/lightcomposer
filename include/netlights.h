#ifndef NETLIGHTS_H_
#define NETLIGHTS_H_

#include "blocking_queue.h"

struct netlights_ctx {
  BlockingQueue<Uint32*> *lightsq;
  int n;
};

void netlights_init(netlights_ctx *ctx);

#endif
