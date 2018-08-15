#include <iostream>
#include <math.h>
#include <algorithm>

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
#include <boost/lockfree/queue.hpp>

PixelQueue drawq;

const char* url = "file:sail.mp3";

const int FFT_SAMPLE_SIZE = 4096.0;
const Uint16 SCREEN_W = 1024;
const Uint16 SCREEN_H = 768;
const Uint16 ZERO = 0;


struct sampleset {
    int size;
    double *data;

    sampleset(int capacity) {
        size = capacity;
        data = new double[size];
    }

    sampleset(const sampleset &o) {
        data = new double[o.size];
        memcpy(data, o.data, o.size);
    }

    ~sampleset() {
        delete[] data;
    }

};

boost::lockfree::queue<sampleset*> samples(128);

volatile int quit = 0;

fftw_complex *in, *out;
fftw_plan p;

void fftw_init() {
    in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * FFT_SAMPLE_SIZE);
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * FFT_SAMPLE_SIZE);
    p = fftw_plan_dft_1d(FFT_SAMPLE_SIZE, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
}

void pixel_callback(Uint8 *stream, int desiredLen, int len) {
    int pixel_count = 16;

    int pixels[pixel_count];

    memset(pixels, 0, sizeof(pixels));
    memset(in, 0, 512);

    sampleset *sample = new sampleset(512);

    // get mono samples assuming 16-bit LRLR..LR order
    for (int i = 0; i < desiredLen + len; i+=4) {
        int16_t l_sample = *(int16_t *)&stream[i];
        int16_t r_sample = *(int16_t *)&stream[i+2];

        int16_t mono = (int(l_sample) + r_sample) >> 1;

        int pixel_idx = ((i / 4) / (512 / pixel_count) % pixel_count);
        pixels[pixel_idx] = std::max(pixels[pixel_idx], abs(mono));
        
        sample->data[i / 4] = (double)mono;
    }

    samples.push(sample);

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

int main(int, char**) {
    SDL_Surface* screen = NULL;
    SDL_Surface* gfx = NULL;
    SDL_Surface* spectrogram = NULL;
    SDL_Event event;

    fftw_init();

    init_libs();

    screen = SDL_SetVideoMode(SCREEN_W, SCREEN_H, 32, SDL_SWSURFACE);

    gfx = getLayer(screen);
    spectrogram = getLayer(screen);

    SDL_SetAlpha(spectrogram, SDL_SRCALPHA, 128.0);

    audio_play_source(url, (int*)&quit, pixel_callback);

    Uint16 offset = 0;
    int x = 0;

    int fft_in_idx = 0;
        
    pixel_queue_init(&drawq);

    while (!quit) {

        bool draw = false;
        Pixel pixels[SCREEN_W];
        memset(pixels, 0, sizeof(pixels));

        while(pixel_queue_get(&drawq, pixels, SCREEN_W)) {
            for (int i = 0; i< SCREEN_W; ++i) {
                if (pixels[i].value == 0) break;
                float volume = (240.0 * ((float)pixels[i].value / INT16_MAX));
                offset++;
                if (offset >= SCREEN_W) offset = 0;
                lineColor(gfx, offset, 0, offset, SCREEN_H, 0x000000FF);
                lineColor(gfx, offset, 240-volume, offset, 240+volume, 0xFFFFFFFF);
            }
        }

        SDL_Rect source_left = {0, 0, offset, SCREEN_H};
        SDL_Rect source_right = {(Sint16)offset, 0, (Uint16)(SCREEN_W - offset), SCREEN_H};
        SDL_Rect dest_left = {(Sint16)(SCREEN_W-offset), 0, offset, SCREEN_H};
        SDL_Rect dest_right = {0, 0, (Uint16)(SCREEN_W - offset), SCREEN_H};
         
        //SDL_BlitSurface(gfx, &source_left, screen, &dest_left);
        //SDL_BlitSurface(gfx, &source_right, screen, &dest_right);

        sampleset *sample;

        while (samples.pop(sample)) {
            if (fft_in_idx < FFT_SAMPLE_SIZE) {
                int size = std::min(sample->size, FFT_SAMPLE_SIZE - fft_in_idx);
                for (int i = 0; i < size; ++i) {
                    in[fft_in_idx][0] = sample->data[i];
                    in[fft_in_idx++][1] = 0.0;
                }
            } else {
                std::cout << "Oops! dropping samples" << std::endl;
            }
            delete sample;
        }

        
        if (fft_in_idx >= FFT_SAMPLE_SIZE) {
            fft_in_idx = 0;
            fftw_execute(p);
            SDL_Rect fill = {0, 0, SCREEN_W, SCREEN_H};
            SDL_FillRect(spectrogram, &fill, 0xFF000000);

            double c0 = 16.35;
            for (int i = 0; i < 20; ++i) {
                int freq = pow(2.0, i) * c0;          
                int x = freq / 11025.0 * 1024.0;
                std::cout << x << std::endl;
                lineColor(spectrogram, x, 0, x, SCREEN_H, 0x00FF00FF);
            }

            for (int i = 0; i < FFT_SAMPLE_SIZE >> 1; ++i) {
                if (i > SCREEN_W) break;
                float vol =(sqrt(out[i][0]*out[i][0] + out[i][1]*out[i][1])) / 5012.0;
                lineColor(spectrogram, i, SCREEN_H, i, SCREEN_H-vol, 0xFF0000FF);
            }
            x++;
            if (x >= SCREEN_W) x = 0; 
            
            SDL_BlitSurface(spectrogram, &fill, screen, &fill);
        }

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
