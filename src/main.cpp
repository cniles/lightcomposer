#include <iostream>
#include <math.h>
#include <algorithm>

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

const char* url = "file:ukulele.mp3";

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
    sampleset *sample = new sampleset(512);
    for (int i = 0; i < desiredLen + len; i+=4) {
        int16_t l_sample = *(int16_t *)&stream[i];
        int16_t r_sample = *(int16_t *)&stream[i+2];
        int16_t mono = (int(l_sample) + r_sample) >> 1;
        //sample->data[i / 4] = (double)mono;
    }
    delete sample;
    //samples.push(sample);
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
        640, 480,
        SDL_WINDOW_SHOWN);

    std ::cout << "Creating surfaces" << std::endl;

    std::cout << "Starting audio" << std::endl;
    audio_play_source(url, (int*)&quit, pixel_callback);

    Uint16 offset = 0;
    int x = 0;

    int fft_in_idx = 0;
        
    while (!quit) {

        bool draw = false;
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
            //SDL_Rect fill = {0, 0, SCREEN_W, SCREEN_H};
            //SDL_FillRect(spectrogram, &fill, 0xFF000000);

            double c0 = 16.35;
            for (int i = 0; i < 20; ++i) {
                int freq = pow(2.0, i) * c0;          
                int x = freq / 11025.0 * 1024.0;
                std::cout << x << std::endl;
                //lineColor(screen, x, 0, x, SCREEN_H, 0x00FF00FF);
            }

            for (int i = 0; i < FFT_SAMPLE_SIZE >> 1; ++i) {
                if (i > SCREEN_W) break;
                float vol =(sqrt(out[i][0]*out[i][0] + out[i][1]*out[i][1])) / 5012.0;
                //lineColor(spectrogram, i, SCREEN_H, i, SCREEN_H-vol, 0xFF0000FF);
            }
            x++;
            if (x >= SCREEN_W) x = 0; 
            
            //SDL_BlitSurface(spectrogram, &fill, screen, &fill);
        }

        //SDL_Flip(screen);           

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
