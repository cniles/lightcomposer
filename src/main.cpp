#include <iostream>
#include <math.h>
#include <algorithm>
#include <wiringPi.h>

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

const int FFT_SAMPLE_SIZE = 4096.0;
const Uint16 SCREEN_W = 1024;
const Uint16 SCREEN_H = 768;
const Uint16 ZERO = 0;

#define LIGHTS 8

/*

 +-----+-----+---------+------+---+-Pi ZeroW-+---+------+---------+-----+-----+
 | BCM | wPi |   Name  | Mode | V | Physical | V | Mode | Name    | wPi | BCM |
 +-----+-----+---------+------+---+----++----+---+------+---------+-----+-----+
 |     |     |    3.3v |      |   |  1 || 2  |   |      | 5v      |     |     |
 |   2 |   8 |   SDA.1 |   IN | 1 |  3 || 4  |   |      | 5v      |     |     |
 |   3 |   9 |   SCL.1 |   IN | 1 |  5 || 6  |   |      | 0v      |     |     |
 |   4 |   7 | GPIO. 7 |   IN | 1 |  7 || 8  | 1 | ALT5 | TxD     | 15  | 14  |
 |     |     |      0v |      |   |  9 || 10 | 1 | ALT5 | RxD     | 16  | 15  |
 |  17 |   0 | GPIO. 0 |   IN | 0 | 11 || 12 | 0 | ALT0 | GPIO. 1 | 1   | 18  |
 |  27 |   2 | GPIO. 2 |   IN | 0 | 13 || 14 |   |      | 0v      |     |     |
 |  22 |   3 | GPIO. 3 |   IN | 0 | 15 || 16 | 0 | IN   | GPIO. 4 | 4   | 23  |
 |     |     |    3.3v |      |   | 17 || 18 | 0 | IN   | GPIO. 5 | 5   | 24  |
 |  10 |  12 |    MOSI |   IN | 0 | 19 || 20 |   |      | 0v      |     |     |
 |   9 |  13 |    MISO |   IN | 0 | 21 || 22 | 0 | IN   | GPIO. 6 | 6   | 25  |
 |  11 |  14 |    SCLK |   IN | 0 | 23 || 24 | 1 | IN   | CE0     | 10  | 8   |
 |     |     |      0v |      |   | 25 || 26 | 1 | IN   | CE1     | 11  | 7   |
 |   0 |  30 |   SDA.0 |   IN | 1 | 27 || 28 | 1 | IN   | SCL.0   | 31  | 1   |
 |   5 |  21 | GPIO.21 |   IN | 1 | 29 || 30 |   |      | 0v      |     |     |
 |   6 |  22 | GPIO.22 |   IN | 1 | 31 || 32 | 0 | IN   | GPIO.26 | 26  | 12  |
 |  13 |  23 | GPIO.23 |   IN | 0 | 33 || 34 |   |      | 0v      |     |     |
 |  19 |  24 | GPIO.24 | ALT0 | 0 | 35 || 36 | 0 | IN   | GPIO.27 | 27  | 16  |
 |  26 |  25 | GPIO.25 |   IN | 0 | 37 || 38 | 0 | ALT0 | GPIO.28 | 28  | 20  |
 |     |     |      0v |      |   | 39 || 40 | 0 | ALT0 | GPIO.29 | 29  | 21  |
 +-----+-----+---------+------+---+----++----+---+------+---------+-----+-----+
 | BCM | wPi |   Name  | Mode | V | Physical | V | Mode | Name    | wPi | BCM |
 +-----+-----+---------+------+---+-Pi ZeroW-+---+------+---------+-----+-----+
 */
const int light_pins[] = { 22,21,3,2,0,7,9,8, 5,6,26,27 };

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
        if ((i / 4) > sample->size) continue;
        sample->data[i / 4] = (double)mono;
    }
    samples.push(sample);
}

void init_libs() {
    av_register_all();
    if(SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        abort();
    }
}

int main(int argc, char** argv) {

  std::cout << "Starting wiring pi" << std::endl;
  if (wiringPiSetup() == -1) {
    std::cout << "Error starting wiring pi" << std::endl;
    return 1;
  } else {
    for(int i = 0; i < LIGHTS; ++i) pinMode(light_pins[i], OUTPUT);
  }

    SDL_Event event;

    const char *url = argv[1];

    const float threshold = atof(argv[2]);

    std::cout << "Playing file " << url << std::endl;

    std::cout << "Starting fftw" << std::endl;
    fftw_init();

    std::cout << "Init libs" << std::endl;
    init_libs();

    /*SDL_Window *screen = SDL_CreateWindow("LightComposer",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        SCREEN_W, SCREEN_H,
        SDL_WINDOW_SHOWN);*/

    //SDL_Renderer *renderer = SDL_CreateRenderer(screen, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

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
	  //            SDL_RenderClear(renderer);
            bool draw = false;
            fft_in_idx = 0;
            fftw_execute(p);

            int lights[LIGHTS];

            memset(lights, 1, sizeof(lights));
            
	    //            SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	    //            SDL_Rect fill = {0, 0, SCREEN_W, SCREEN_H};
	    //            SDL_RenderFillRect(renderer, &fill);

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
	    //            SDL_SetRenderDrawColor(renderer, 0xFF, 0, 0, SDL_ALPHA_OPAQUE);
            for (int i = 0; i < 10; ++i) {
                int freq = pow(2.0, i) * c0;
                int x = log2(freq) / 26.0 * SCREEN_W;
		//                SDL_RenderDrawLine(renderer, x, 0, x, SCREEN_H);
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

                if (r > threshold) {
                    lights[o] = 0;
                }

		//                SDL_SetRenderDrawColor(renderer, 0, 0xFF * (1.0 - r), 0xFF*r, SDL_ALPHA_OPAQUE);
		//                SDL_RenderDrawLine(renderer, x, SCREEN_H, x, SCREEN_H-vol);
            }

	    for (int i = 0; i < LIGHTS; ++i) {
	      std::cout << i << ":" << lights[i] << " ";
	        digitalWrite(light_pins[i], lights[i]);
	    }
	    std::cout << std::endl;

           /*for (int i = 0; i < LIGHTS; ++i) {
                if (lights[i]) {
                    filledCircleColor(renderer, 50+50*i, 50, 20, 0xFFFFFFFF);
                } else {
                    filledCircleColor(renderer, 50+50*i, 50, 20, 0xAAAAAAFF);
                }
		}*/
            
	    //            SDL_RenderPresent(renderer);
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
	//        SDL_UpdateWindowSurface(screen);
    }

    fftw_destroy_plan(p);
    fftw_free(in);
    fftw_free(out);
}
