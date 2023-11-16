#include <algorithm>
#include <cstdlib>
#include <fftw3.h>
#include <iostream>
#include <math.h>

#include "sampleset.h"
#include "mstime.h"
#include "blocking_queue.h"
#include "draw.h"
#include "netlights.h"

#ifdef WIRINGPI
#include <wiringPi.h>
#endif

// Need this for CPlusPlus development with this C library
#ifdef __cplusplus
extern "C" {
  #include <SDL.h>
}
#include "draw.h"
#endif
#ifdef SDLGFX
#define VIDEO_INIT SDL_INIT_VIDEO
#else
#define VIDEO_INIT 0
#endif

#include "audio.h"
#include "draw.h"
#include "music.h"
#include <boost/lockfree/queue.hpp>
#include <fftw3.h>
#include <unistd.h>

const int SAMPLE_RATE = 44100;
const int FFT_SAMPLE_SIZE = 882;
const int N = SAMPLE_RATE / FFT_SAMPLE_SIZE;
const Uint16 ZERO = 0;

#define LIGHTS 12
#define OCTAVE_BANDS 10

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
const int light_pins[] = {22, 21, 3, 2, 0, 7, 9, 8, 5, 6, 26, 27};

boost::lockfree::queue<sampleset *> samples(128);

volatile int quit = 0;

fftw_complex *in, *out;
fftw_plan p;

void fftw_init() {
  in = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * FFT_SAMPLE_SIZE);
  out = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * FFT_SAMPLE_SIZE);
  p = fftw_plan_dft_1d(FFT_SAMPLE_SIZE, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
}

void fft_callback(float *input, int nb_samples) {
  sampleset *sample = new sampleset(nb_samples);
  for (int i = 0; i < nb_samples; ++i) {
    sample->data[i] = (double)input[i];
  }
  samples.push(sample);
}

void init_libs() {
  av_register_all();
  if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER | VIDEO_INIT)) {
    abort();
  }
}

/**
 * Iterate over FFT samples and calcuate power for each of
 * them; sums the power for each frequency into an octave
 * bin to calculate the strength of each octave, and the total
 * power of the signal.
 */
void calc_power(double *power, double *freqs, int *band, double *band_power,
                int len, int band_count, double *max) {
  int b = 0; 
  double b_freq = C0;

  *max = 0.0;

  for (int i = 0; i<FFT_SAMPLE_SIZE>> 1; ++i) {
    // calculate frequency for output
    // (http://www.fftw.org/fftw3_doc/What-FFTW-Really-Computes.html) "the

    // k-th output corresponds to the frequency k/n (or k/T, where T is your
    // total sampling period)".
    freqs[i] = (double)i / ((double)FFT_SAMPLE_SIZE / 44100.0);
    if (b_freq < freqs[i]) {
      b_freq = pow(2.0, ++b) * C0;
    }
    power[i] = sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]);
    *max = fmax(*max, power[i]);
    if (b < band_count) {
      band[i] = b;
      band_power[b] += power[i];
    } else {
      band[i] = -1;
    }
  }
}

int main(int argc, char *argv[]) {
  BlockingQueue<Uint32*> lights_queue;

  long start_time = next_half_second();

  std::cout << "Starting music at " << start_time << std::endl;

#ifdef WIRINGPI
  std::cout << "Starting wiring pi" << std::endl;
  if (wiringPiSetup() == -1) {
    std::cout << "Error starting wiring pi" << std::endl;
    return 1;
  } else {
    for (int i = 0; i < LIGHTS; ++i)
      pinMode(light_pins[i], OUTPUT);
  }
#endif

  SDL_Event event;

  const char *url = argv[1];

  const double threshold = atof(argv[2]);

  bool packet_queue_loaded = false;

  std::cout << "Playing file " << url << std::endl;

  std::cout << "Starting fftw" << std::endl;
  fftw_init();

  std::cout << "Init libs" << std::endl;
  init_libs();

  draw_init();

  std ::cout << "Creating surfaces" << std::endl;

  std::cout << "Starting audio" << std::endl;
  if (audio_play_source(url, fft_callback, &packet_queue_loaded, start_time)) {
    std::cout << "audio setup failed" << std::endl;
    return(1);
  }

  netlights_ctx nl_ctx;

  nl_ctx.lightsq = &lights_queue;
  nl_ctx.n = N;

  netlights_init(&nl_ctx);
   
  std::cout << "Starting audio returned" << std::endl;

  int fft_in_idx = 0;

  double max;

  int beats = 0;
  long next_broadcast = start_time - 500;
  Uint32 lights_buffer[N];
  Uint32 lights_buffer_idx;

  memset(&lights_buffer, 0, sizeof(lights_buffer));
    
  while (!quit) {

    sampleset *sample;
    
    while (fft_in_idx < FFT_SAMPLE_SIZE && samples.pop(sample)) {
      if (fft_in_idx < FFT_SAMPLE_SIZE) {
        int size = std::min(sample->size, FFT_SAMPLE_SIZE - fft_in_idx);
        // copy the fft samples out of the sampleset into our FFT input array of complex numbers.
        for (int i = 0; i < size; ++i) {
          in[fft_in_idx][0] = sample->data[i];
          in[fft_in_idx++][1] = 0.0;
        }
      } else {
        std::cout << "Oops! dropping samples" << std::endl;
      }
      delete sample;
    }

    if (samples.empty() && audio_queue_empty() && packet_queue_loaded) {
      // all packets have been added to the queue and we're no longer getting data, must be finished.
      quit = 1;
    }

    // have we read enough samples?
    if (fft_in_idx >= FFT_SAMPLE_SIZE) {
      draw_begin_frame();
      draw_octave_markers();

      fft_in_idx = 0;
      fftw_execute(p);

      Uint32 lights = 0;
      double power[FFT_SAMPLE_SIZE >> 1];
      double freqs[FFT_SAMPLE_SIZE >> 1];
      int band[FFT_SAMPLE_SIZE >> 1];
      double band_power[OCTAVE_BANDS];

      memset(band_power, 0, sizeof(double) * OCTAVE_BANDS);

      calc_power(power, freqs, band, band_power, FFT_SAMPLE_SIZE >> 1,
                 OCTAVE_BANDS, &max);

      int light_freq_count[LIGHTS];
      memset(light_freq_count, 0, sizeof(light_freq_count));
      for (int i = 0; i < FFT_SAMPLE_SIZE >> 1; ++i) {
        // get the total power for the band
        if (band[i] < 0)
          break;
        double q = band_power[band[i]];
        double r = power[i] / q;

        // Map the frequency back to an approximate note
        double note = (log2(freqs[i] / C0) * 12.0);
        int o = (int)note % LIGHTS;

        // if the frequency is strong compared to the rest of its octave, toggle
        // that notes light
        if (power[i] > 5 && power[i] > max * threshold) {
          lights |= 1 << o;
          light_freq_count[o]++;
        }

        draw_frequency(freqs[i], power[i], q);
      }

      int offset = (beats * FFT_SAMPLE_SIZE) % SAMPLE_RATE;

      // This will dump the light states to stdout.
      // for (int i = 0; i < LIGHTS; ++i) {
      //   int state = (lights >> i) & 1;
      //   std::cout << state << ", ";
      // }
      // std::cout << offset / 44100.0;  
      // std::cout << std::endl;

      lights_buffer[lights_buffer_idx] = lights;
      lights_buffer_idx++;
      
      beats++;

      draw_lights(lights, LIGHTS);
      draw_end_frame();
    }

    // std::cout << lights_buffer_idx << std::endl;

    if (lights_buffer_idx >= N || audio_queue_empty()) {
      // wait until next_broadcast elapses
      long delay = next_broadcast - millis_since_epoch();

      if (delay > 0) {
        SDL_Delay(delay);
      }

      std::cout << "Broadcasting next chunk of lights" << std::endl;

      // push our 200 samples onto the blocking queue (after copying into a new block)
      // TBD: after popping from queue
      {
        Uint32 *copy = new Uint32[N];
        memcpy(lights_buffer, copy, N);
        lights_queue.push(copy);
      }
      
      memcpy(lights_buffer, lights_buffer + (N/2), (N/2));

      next_broadcast += 500;

      lights_buffer_idx = N / 2;
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
  }

  std::cout << "Terminating" << std::endl;

  fftw_destroy_plan(p);
  fftw_free(in);
  fftw_free(out);
}
