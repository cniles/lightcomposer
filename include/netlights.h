#ifndef NETLIGHTS_H_
#define NETLIGHTS_H_

#include "blocking_queue.h"

void start_netlights(BlockingQueue<Uint32*> *lightsq, int n);

#endif
