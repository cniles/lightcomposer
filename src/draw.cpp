#include "draw.h"

#ifdef SDLGFX

#include "music.h"
#include <SDL2_gfxPrimitives.h>

const Uint16 SCREEN_W = 1024;
const Uint16 SCREEN_H = 768;

static SDL_Renderer *renderer = NULL;
static SDL_Window *screen = NULL;

void _draw_octive_markers() {
  SDL_SetRenderDrawColor(renderer, 0xFF, 0, 0, SDL_ALPHA_OPAQUE);
  for (int i = 0; i < 10; ++i) {
    int freq = pow(2.0, i) * C0;
    int x = log2(freq) / 26.0 * SCREEN_W;
    SDL_RenderDrawLine(renderer, x, 0, x, SCREEN_H);
  }
}

void _draw_init() {
  screen = SDL_CreateWindow("LightComposer", SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED, SCREEN_W, SCREEN_H,
                            SDL_WINDOW_SHOWN);
  renderer = SDL_CreateRenderer(
      screen, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
}

void _draw_begin_frame() {
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
  SDL_Rect fill = {0, 0, SCREEN_W, SCREEN_H};
  SDL_RenderFillRect(renderer, &fill);
}

void _draw_frequency(double freq, double freq_power, double band_power) {
  double x = log2(freq) / 26.0 * SCREEN_W;
  SDL_SetRenderDrawColor(renderer, 0, 0xFF * (1.0 - freq_power / band_power), 0,
                         SDL_ALPHA_OPAQUE);
  SDL_RenderDrawLine(renderer, x, SCREEN_H, x, SCREEN_H - freq_power);
}

void _draw_lights(int *lights, int len) {
  for (int i = 0; i < len; ++i) {
    if (lights[i]) {
      filledCircleColor(renderer, 50 + 50 * i, 50, 20, 0xFFFFFFFF);
    } else {
      filledCircleColor(renderer, 50 + 50 * i, 50, 20, 0xAAAAAAFF);
    }
  }
}

void _draw_end_frame() { SDL_RenderPresent(renderer); }

#endif