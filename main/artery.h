#ifndef __ARTERY__
#define __ARTERY__

#include "stdint.h"

typedef struct {
	uint8_t pulse_width;
	uint8_t pulse_gap;
	uint8_t pulse_bpm;
	uint8_t pulse_flow;
	float pulse_hue;
	float pulse_light_min;
	float pulse_light_max;
} beat_parameters_t;

enum DMX_CHANNEL {
	DMX_WIDTH,
	DMX_GAP,
	DMX_BPM,
	DMX_FLOW,
	DMX_HUE,
	DMX_LIGHT_MIN,
	DMX_LIGHT_MAX,
};

beat_parameters_t beat_parameters;

void init_artery_values();
void artery_tick();

#endif  // __ARTERY__

