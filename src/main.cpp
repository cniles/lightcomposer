#include <iostream>
#include <math.h>

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
#include <fftw3.h>

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

fftw_complex *in, *out;
fftw_plan p;

void fftw_init() {
    in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * 512);
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * 512);
    p = fftw_plan_dft_1d(512, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
}

SDL_mutex *ftlock;

void pixel_callback(Uint8 *stream, int desiredLen, int len) {
    int pixel_count = 16;

    int pixels[pixel_count];

    memset(pixels, 0, sizeof(pixels));
    memset(in, 0, 512);

    SDL_LockMutex(ftlock);
    
    // get mono samples assuming 16-bit LRLR..LR order
    for (int i = 0; i < desiredLen + len; i+=4) {
        int16_t l_sample = *(int16_t *)&stream[i];
        int16_t r_sample = *(int16_t *)&stream[i+2];

        int pixel_idx = ((i / 4) / (512 / pixel_count) % pixel_count);

        pixels[pixel_idx] = max(pixels[pixel_idx], abs((int(l_sample) + r_sample) >> 1));

        int fft_idx = (i / 4);

        in[fft_idx][0] = (double)pixels[pixel_idx];
    }
    
    SDL_UnlockMutex(ftlock);

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

SDL_Surface *getLayer(SDL_Surface *screen) {
 SDL_CreateRGBSurface(
        SDL_SWSURFACE, 
        screen->w,
        screen->h,
        screen->format->BitsPerPixel,
        screen->format->Rmask,
        screen->format->Gmask,
        screen->format->Bmask,
        screen->format->Amask);
}

int main(int, char**)
{
    SDL_Surface* screen = NULL;
    SDL_Surface* gfx = NULL;
    SDL_Surface* spectrogram = NULL;
    SDL_Event event;

    fftw_init();

    init_libs();

    screen = SDL_SetVideoMode(640, 480, 32, SDL_SWSURFACE);

    ftlock = SDL_CreateMutex();

    gfx = getLayer(screen);
    spectrogram = getLayer(screen);

    SDL_SetAlpha(spectrogram, SDL_SRCALPHA, 128.0);

    const char* url = "file:sail.mp3";
    audio_play_source(url, (int*)&quit, pixel_callback);

    Uint16 offset = 0;
    int x = 0;

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

        SDL_LockMutex(ftlock);
        fftw_execute(p);

        lineColor(spectrogram, x, 0, x, 480, 0x000000FF);
        for (int i = 0; i < 512; ++i) {
            float vol = (sqrt(out[i][0]*out[i][0] + out[i][1]*out[i][1])) / 100000.0;
            int y = ((i / 512.0) * 480);
            pixelColor(spectrogram, x, y, 0xFFFFFF00 | (int)(vol * 256) );
        }
        x++;
        if (x >= 640) x = 0;
        SDL_UnlockMutex(ftlock);
        
        SDL_Rect fill = {0, 0, 640, 480};

        SDL_BlitSurface(spectrogram, &fill, screen, &fill);

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

    fftw_destroy_plan(p);
    fftw_free(in);
    fftw_free(out);
}
