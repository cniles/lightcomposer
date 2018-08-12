#include "pixel.h"

void pixel_queue_init(PixelQueue* q) {
    memset(q, 0, sizeof(PixelQueue));
    q->mutex = SDL_CreateMutex();
    q->cond = SDL_CreateCond();
}

int pixel_queue_get(PixelQueue * q, Pixel pixels[], int size) {
    int ret = 0;
    SDL_LockMutex(q->mutex);
    int dropped = 0;
    while (dropped < size && q->first_pixel != NULL) {
        Pixel *pixel = &pixels[dropped];
        if (q->first_pixel != NULL) {
            Pixel *pixel0 = q->first_pixel;
            *pixel = *pixel0;
            q->first_pixel = pixel->next;
            if (pixel->next == NULL) {
                q->last_pixel = NULL;
            }
            q->size--;
            dropped++;
            delete pixel0;
            ret = 1;
        }
    }
    SDL_UnlockMutex(q->mutex);

    return ret;
}

int pixel_queue_put(PixelQueue* q, int val) {
    SDL_LockMutex(q->mutex);
    
    Pixel *pixel0 = new Pixel();

    pixel0->next = NULL;
    pixel0->value = val;

    if (q->last_pixel == NULL) {
        q->first_pixel = pixel0;
    } else {
        q->last_pixel->next = pixel0;
    }
    q->last_pixel = pixel0;
    q->size++;
    SDL_UnlockMutex(q->mutex);
    return 0;
}

