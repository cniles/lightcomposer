#ifndef DRAW_H
#define DRAW_H

#ifdef SDLGFX
#define draw_octave_markers _draw_octave_markers
#define draw_init _draw_init
#define draw_begin_frame _draw_begin_frame
#define draw_frequency _draw_frequency
#define draw_lights _draw_lights
#define draw_end_frame _draw_end_frame
#else
#define draw_octave_markers()
#define draw_init()
#define draw_begin_frame()
#define draw_frequency(a, b, c)
#define draw_lights(a, b)
#define draw_end_frame()
#endif

#ifdef SDLGFX
void _draw_octave_markers();
void _draw_init();
void _draw_begin_frame();
void _draw_frequency(double freq, double freq_power, double band_power);
void _draw_lights(int *lights, int len);
void _draw_end_frame();
#endif

#endif