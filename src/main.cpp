#include <iostream>
#include <math.h>
#include <algorithm>
#include <SDL2_gfxPrimitives.h>

// Need this for CPlusPlus development with this C library
#ifdef __cplusplus
extern "C"
{
    #include <SDL.h>
}
#endif

#include <unistd.h>
#include "packet_queue.h"
#include "audio.h"
#include <fftw3.h>
#include <boost/lockfree/queue.hpp>

const char* url = "file:Peanuts-Theme.mp3";

const int FFT_SAMPLE_SIZE = 4096.0;
const Uint16 SCREEN_W = 1024;
const Uint16 SCREEN_H = 768;
const Uint16 ZERO = 0;

#define LIGHTS 8

struct sampleset {
    int size;
    double *data;

    sampleset(int capacity) {
        size = capacity;
        data = (double*)malloc(sizeof(double) * capacity);
    }

    sampleset(const sampleset &o) {
        data = new double[o.size];
        memcpy(data, o.data, o.size);
    }

    ~sampleset() {
        free(data);
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
    sampleset *sample = new sampleset((desiredLen + len) / 4);
    for (int i = 0; i < desiredLen + len; i+=4) {
        if (i+2 >= desiredLen + len) continue;
        int16_t l_sample = *(int16_t *)&stream[i];
        int16_t r_sample = *(int16_t *)&stream[i+2];
        int16_t mono = (int(l_sample) + r_sample) >> 1;
        if (i < sample->size) continue;
        sample->data[i / 4] = (double)mono;
    }
    samples.push(sample);
}

void init_libs() {
    av_register_all();
    if(SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_VIDEO)) {
        abort();
    }
}

int main(int, char**) {
    SDL_Event event;

    std::cout << "Starting fftw" << std::endl;
    fftw_init();

    std::cout << "Init libs" << std::endl;
    init_libs();

    SDL_Window *screen = SDL_CreateWindow("LightComposer",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        SCREEN_W, SCREEN_H,
        SDL_WINDOW_SHOWN);

    SDL_Renderer *renderer = SDL_CreateRenderer(screen, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    std ::cout << "Creating surfaces" << std::endl;

    std::cout << "Starting audio" << std::endl;
    audio_play_source(url, (int*)&quit, pixel_callback); 

    Uint16 offset = 0;
    int fft_in_idx = 0;
        
    while (!quit) {

        sampleset *sample;
        while (fft_in_idx < FFT_SAMPLE_SIZE && samples.pop(sample)) {
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
            SDL_RenderClear(renderer);
            bool draw = false;
            fft_in_idx = 0;
            fftw_execute(p);

            int lights[LIGHTS];

            memset(lights, 0, sizeof(lights));
            
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
            SDL_Rect fill = {0, 0, SCREEN_W, SCREEN_H};
            SDL_RenderFillRect(renderer, &fill);

            double c0 = 16.35;            

            double power[FFT_SAMPLE_SIZE >> 1];
            double freqs[FFT_SAMPLE_SIZE >> 1];
            double bands[10];

            memset(bands, 0, sizeof(bands));
            
            int b = 0;
            int band_bin_counter = 0;
            int b_freq = c0;
            double total = 0.0;
            for (int i = 0; i < FFT_SAMPLE_SIZE >> 1; ++i) {
                freqs[i] = (double)i / ((double)FFT_SAMPLE_SIZE / 44100.0);
                if (b_freq < freqs[i]) {
                    bands[b] /= (double)band_bin_counter;
                    b++;
                    b_freq = pow(2.0, b) * c0;
                    band_bin_counter = 0;
                }
                power[i] = sqrt(out[i][0]*out[i][0] + out[i][1]*out[i][1]);
                bands[b] += power[i];
                band_bin_counter++;
                total += power[i];
            }
            total /= (FFT_SAMPLE_SIZE / 2.0);

            // Draw octave
            SDL_SetRenderDrawColor(renderer, 0xFF, 0, 0, SDL_ALPHA_OPAQUE);
            for (int i = 0; i < 10; ++i) {
                int freq = pow(2.0, i) * c0;
                int x = log2(freq) / 26.0 * SCREEN_W;
                SDL_RenderDrawLine(renderer, x, 0, x, SCREEN_H);
            }

            b = 0;
            b_freq = c0;
            double s = 30.0;
            for (int i = 0; i < FFT_SAMPLE_SIZE >> 1; ++i) {

                if (b_freq < freqs[i]) {
                    b_freq = pow(2.0, b) * c0;
                    b++;
                }

                if (i > SCREEN_W) break;

                double q = bands[b];
              
                double r = std::min(s, power[i] / q) / s;

                int vol = power[i] / 10000;

                int o = (int)(log2(freqs[i] / c0) * 12) % LIGHTS;

                double x = log2(freqs[i]) / 26.0 * SCREEN_W;

                if (r > 0.5) {
                    lights[o] = 1;
                }

                SDL_SetRenderDrawColor(renderer, 0, 0xFF * (1.0 - r), 0xFF*r, SDL_ALPHA_OPAQUE);
                SDL_RenderDrawLine(renderer, x, SCREEN_H, x, SCREEN_H-vol);
            }

            for (int i = 0; i < LIGHTS; ++i) {
                if (lights[i]) {
                    filledCircleColor(renderer, 50+50*i, 50, 20, 0xFFFFFFFF);
                } else {
                    filledCircleColor(renderer, 50+50*i, 50, 20, 0xAAAAAAFF);
                }
            }
            
            SDL_RenderPresent(renderer);
        }


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
        SDL_UpdateWindowSurface(screen);
    }

    fftw_destroy_plan(p);
    fftw_free(in);
    fftw_free(out);
}
