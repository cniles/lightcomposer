#ifndef LISTENER_H_
#define LISTENER_H_

#include <SDL2/SDL_thread.h>

SDL_Thread *listen_for_traffic(SDL_cond *ready_cond); 

#endif
