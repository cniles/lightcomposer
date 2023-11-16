#ifndef BLOCKING_QUEUE_H_
#define BLOCKING_QUEUE_H_

#include <list>

extern "C" {
    #include <SDL.h>
}

template <class T>
class BlockingQueue {
private:
  SDL_mutex *mutex;
  SDL_cond *cond;
  std::list<T> list;
  bool closed;
public:
  ~BlockingQueue();
  BlockingQueue();

  void push(T item);
  int pop(T *ret_val);
  void close();
  bool empty();
  bool is_closed();
};

template <class T>
BlockingQueue<T>::BlockingQueue() {
  mutex = SDL_CreateMutex();
  cond = SDL_CreateCond();
  closed = false;
}

template <class T>
void BlockingQueue<T>::push(T item) {
  SDL_LockMutex(mutex);

  if (!closed) {
    list.push_back(item);
    SDL_CondSignal(cond);
  }

  SDL_UnlockMutex(mutex);
}

template <class T>
int BlockingQueue<T>::pop(T *val) {
  SDL_LockMutex(mutex);

  if (list.empty() && !closed) {
    SDL_CondWait(cond, mutex);
  }

  if (!list.empty()) {
    T result = *(list.begin());
    list.erase(list.begin());
    *val = result;
    SDL_UnlockMutex(mutex);
    return 1;
  }

  SDL_UnlockMutex(mutex);
  return -1;
}

template <class T>
void BlockingQueue<T>::close() {
  SDL_LockMutex(mutex);
  closed = true;
  SDL_CondSignal(cond);
  SDL_UnlockMutex(mutex);
}

template <class T>
bool BlockingQueue<T>::empty() {
  SDL_LockMutex(mutex);
  bool empty = list.empty();
  SDL_UnlockMutex(mutex);
  return empty;
}

template <class T>
bool BlockingQueue<T>::is_closed() {
  bool result;

  SDL_LockMutex(mutex);
  result = closed;
  SDL_UnlockMutex(mutex);

  return result;
}

template <class T>
BlockingQueue<T>::~BlockingQueue() {
  SDL_DestroyCond(cond);
  SDL_DestroyMutex(mutex);
}

#endif