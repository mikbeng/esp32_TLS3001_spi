#include "artery.h"

#include "stdint.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "settings.h"
#include "pattern_generator.h"
#include "e131.h"

static const char * TAG = "artery";

uint8_t bpm_last = 0;
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
	beat_parameters.pulse_width = 18;
	beat_parameters.pulse_gap = 40;
	beat_parameters.pulse_bpm = 128;
	beat_parameters.pulse_flow = 70;
	beat_parameters.pulse_hue = 0.0;
	beat_parameters.pulse_light_min = 0.0;
	beat_parameters.pulse_light_max = 0.1;
}

beat_parameters_t readparametersfromdmx() {
	if (e131packet.universe != settings.universeStart) {
		//ESP_LOGI(TAG, "DMX Universe %d, looking for %d", e131packet.universe, settings.universeStart);
		// If not configured universe, return parameters as is
		return beat_parameters;
	}

	int dmx_address = settings.dmx_start;

	beat_parameters_t paramout;
	// Read parameters from DMX values
	paramout.pulse_light_max = (float)e131packet.property_values[dmx_address+DMX_LIGHT_MAX] / 255;
	paramout.pulse_light_min = ((float)e131packet.property_values[dmx_address+DMX_LIGHT_MIN] / 255) * paramout.pulse_light_max;
	paramout.pulse_hue = (float)e131packet.property_values[dmx_address+DMX_HUE] / 255;
	paramout.pulse_width = e131packet.property_values[dmx_address+DMX_WIDTH];
	paramout.pulse_gap = e131packet.property_values[dmx_address+DMX_GAP];
	paramout.pulse_bpm = e131packet.property_values[dmx_address+DMX_BPM];
	paramout.pulse_flow = e131packet.property_values[dmx_address+DMX_FLOW];

	// Make sure values are whithin ranges
	if (paramout.pulse_width < 1) {
		paramout.pulse_width = 1;
	}
	if (paramout.pulse_gap < 1) {
		paramout.pulse_gap = 1;
	}
	if (paramout.pulse_bpm < 1) {
		paramout.pulse_bpm = 1;
	}

	return paramout;
}

void artery_tick() {
	beat_parameters = readparametersfromdmx();

	// Timing of beats
	now = xTaskGetTickCount()*portTICK_PERIOD_MS;
	beatms = (((float)60/beat_parameters.pulse_bpm)*1000)/2;
	// If beat have past, or BPM value changed, calculate next beat
	if (now > beat_end || beat_parameters.pulse_bpm != bpm_last) {
		// Calculate next beat
		beat_mid = now + beatms;
		beat_end = beat_mid + beatms;
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
	// Update last for use in next tick
	bpm_last = beat_parameters.pulse_bpm;

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
			// Light level with beat level, but cap it to light min level
			pixel_lightlevel = max(pixel_lightlevel * beat_level, beat_parameters.pulse_light_min);
		} else {
			pixel_lightlevel = 0;
		}
		setPixelHSL(i, beat_parameters.pulse_hue, 1.0, pixel_lightlevel);
	}
}

