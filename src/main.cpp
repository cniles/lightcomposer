#include <iostream>

// Need this for CPlusPlus development with this C library
#ifdef __cplusplus
extern "C"
{
    #include <SDL/SDL.h>
    #include <SDL/SDL_gfxPrimitives.h>
}
#endif

#include <unistd.h>
#include "packet_queue.h"
#include "pixel.h"
#include "audio.h"

PixelQueue drawq;

volatile int quit = 0;

int16_t abs(int16_t a) {
    if (a >= 0) return a;
    return -a;
}

int16_t max(int16_t a, int16_t b) {
    if (a > b) return a;
    return b;
}

void pixel_callback(Uint8 *stream, int desiredLen, int len) {
    int pixel_count = 16;

    int pixels[pixel_count];

    memset(pixels, 0, sizeof(pixels));

    // get mono samples assuming 16-bit LRLR..LR order
    for (int i = 0; i < desiredLen + len; i+=4) {
        int16_t l_sample = *(int16_t *)&stream[i];
        int16_t r_sample = *(int16_t *)&stream[i+2];

        int pixel_idx = ((i / 4) / (512 / pixel_count) % pixel_count);

        pixels[pixel_idx] = max(pixels[pixel_idx], abs((int(l_sample) + r_sample) >> 1));
    }

    for (int i = 0; i < pixel_count; ++i) {
        pixel_queue_put(&drawq, pixels[i]);
    }    
}

void init_libs() {
    av_register_all();
    if(SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_VIDEO)) {
        abort();
    }
}

const Uint16 SCREEN_W = 640;
const Uint16 SCREEN_H = 480;
const Uint16 ZERO = 0;

int main(int, char**)
{
    SDL_Surface* screen = NULL;
    SDL_Surface* gfx = NULL;
    SDL_Event event;

    init_libs();

    screen = SDL_SetVideoMode(640, 480, 32, SDL_SWSURFACE);

    gfx = SDL_CreateRGBSurface(SDL_SWSURFACE, 640, 480,
        screen->format->BitsPerPixel,
        screen->format->Rmask,
        screen->format->Gmask,
        screen->format->Bmask,
        screen->format->Amask);

    const char* url = "file:killcaustic.mp3";
    audio_play_source(url, (int*)&quit, pixel_callback);

    Uint16 offset = 0;

    pixel_queue_init(&drawq);

    while (!quit) {

        bool draw = false;
        Pixel pixels[640];
        memset(pixels, 0, sizeof(pixels));

        while(pixel_queue_get(&drawq, pixels, 640)) {
            for (int i = 0; i< 640; ++i) {
                if (pixels[i].value == 0) break;
                float volume = (240.0 * ((float)pixels[i].value / INT16_MAX));
                offset++;
                if (offset >= 640) offset = 0;
                lineColor(gfx, offset, 0, offset, 480, 0x000000FF);
                lineColor(gfx, offset, 240-volume, offset, 240+volume, 0xFFFFFFFF);
            }
        }

        SDL_Rect source_left = {0, 0, offset, SCREEN_H};
        SDL_Rect source_right = {(Sint16)offset, 0, (Uint16)(SCREEN_W - offset), SCREEN_H};
        SDL_Rect dest_left = {(Sint16)(640-offset), 0, offset, SCREEN_H};
        SDL_Rect dest_right = {0, 0, (Uint16)(SCREEN_W - offset), SCREEN_H};
        SDL_BlitSurface(gfx, &source_left, screen, &dest_left);
        SDL_BlitSurface(gfx, &source_right, screen, &dest_right);

        SDL_Flip(screen);           

        SDL_PollEvent(&event);
        switch (event.type) {
        case SDL_QUIT:
            quit = 1;
            SDL_Quit();
            exit(0);
        break;
            default:
            break;
        }
    }
}
