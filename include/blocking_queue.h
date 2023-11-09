#ifndef BLOCKING_QUEUE_H_
#define BLOCKING_QUEUE_H_

#include <list>

extern "C" {
    #include <SDL.h>
}

template <class T>
struct BlockingQueue {
  SDL_mutex *mutex;
  SDL_cond *cond;
  std::list<T> list;

  BlockingQueue();  
  void push(T item);
  T pop();
  void close();
  bool empty();
};

template <class T>
BlockingQueue<T>::BlockingQueue() {
  mutex = SDL_CreateMutex();
  cond = SDL_CreateCond();
}

template <class T>
void BlockingQueue<T>::push(T item) {
  SDL_LockMutex(mutex);

  list.push_back(item);

  SDL_UnlockMutex(mutex);
}

template <class T>
T BlockingQueue<T>::pop() {
  SDL_LockMutex(mutex);

  if (list.empty()) {
    SDL_CondWait(cond, mutex);
  }

  T result = *(list.begin());

  list.erase(list.begin());
  
  SDL_UnlockMutex(mutex);

  return result;
}

template <class T>
void BlockingQueue<T>::close() {
  SDL_DestroyCond(cond);
  SDL_DestroyMutex(mutex);
}

template <class T>
bool BlockingQueue<T>::empty() {
  SDL_LockMutex(mutex);
  bool empty = list.empty();
  SDL_UnlockMutex(mutex);
  return empty;
}

#endif