#ifndef PIXEL_H
#define PIXEL_H

#ifdef __cplusplus
extern "C"
{
    #include <SDL/SDL.h>
}
#endif

struct Pixel {
   int value;
   Pixel* next;
};

struct PixelQueue {
    Pixel *first_pixel, *last_pixel;
    int size;
    SDL_mutex *mutex;
    SDL_cond *cond;
};

void pixel_queue_init(PixelQueue* q);
int pixel_queue_get(PixelQueue * q, Pixel pixels[], int size);
int pixel_queue_put(PixelQueue* q, int val);

#endif