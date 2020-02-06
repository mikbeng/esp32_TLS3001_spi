#include "artery.h"

#include "stdint.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "settings.h"
#include "pattern_generator.h"

int segment_pixels;
int segment_index;
float segment_offset = 0.0;
float pixel_lightlevel;
float pixel_rest;
float beat_level;
uint32_t beat_mid = 0;
uint32_t beat_end = 0;
uint32_t now;
uint32_t beatms;
uint32_t diff;

void init_artery_values() {
	beat_parameters.pulse_width = 4;
	beat_parameters.pulse_gap = 10;
	beat_parameters.pulse_bpm = 128;
	beat_parameters.pulse_flow = 30;
	beat_parameters.pulse_hue = 0.0;
	beat_parameters.pulse_light_min = 0.0;
	beat_parameters.pulse_light_max = 0.2;
}

void artery_tick() {
	//beat_parameters = readparametersfromdmx(dmx_address);

	// Calculate beat, and light level on beat
	//printf("Tick %d\n", xTaskGetTickCount()*portTICK_PERIOD_MS);
	now = xTaskGetTickCount()*portTICK_PERIOD_MS;
	beatms = (((float)60/beat_parameters.pulse_bpm)*1000)/2;
	if (now > beat_end) {
		// Calculate next beat
		beat_mid = now + beatms;
		beat_end = beat_mid + beatms;
		//printf("Double beat %d %d\n", beatms, beat_end);
	}
	if (now < beat_mid) {
		// Start of beat, go up
		diff = beat_mid - now;
		beat_level = (float)diff / beatms;
	} else {
		// End of beat, go down
		diff = beat_end - now;
		beat_level = 1.0 - ((float)diff / beatms);
	}
	//printf("Beat level %f\n", beat_level);

	// By offseting segments a movement can be created
	segment_pixels = beat_parameters.pulse_width + beat_parameters.pulse_gap;
	segment_offset += (float)beat_parameters.pulse_flow / 100;
	if (segment_offset > segment_pixels || segment_offset < -segment_pixels) {
		segment_offset = 0;
	}

	// Render
	for (int i = 0; i < settings.pixel_number; i++) {
		segment_index = (abs(i-(int)segment_offset+segment_pixels)) % segment_pixels;
		if (segment_index < beat_parameters.pulse_width) {
			pixel_lightlevel = beat_parameters.pulse_light_max;
			pixel_rest = segment_offset - (int)segment_offset;
			// Fade out/in first/last pixel in segment for smoother transition
			if (segment_index == 0) {
				pixel_lightlevel = pixel_lightlevel * (1-pixel_rest);
			}
			if (segment_index == beat_parameters.pulse_width-1) {
				pixel_lightlevel = pixel_lightlevel * pixel_rest;
			}
			pixel_lightlevel = pixel_lightlevel * beat_level;
		} else {
			pixel_lightlevel = 0;
		}
		setPixelHSL(i, beat_parameters.pulse_hue, 1.0, pixel_lightlevel);
	}
}

