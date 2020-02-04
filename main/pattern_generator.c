#include "main.h"
#include "string.h"
#include "stdint.h"
#include "stdbool.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "TLS3001.h"
#include "pattern_generator.h"
#include "math.h"

#define NUM_LEDS PIXELS_CONNECTED

static const char * TAG = "pattern_gen";

static void pattern_gen_task(void *arg);
static void pattern_equal_color(uint16_t red, uint16_t green, uint16_t blue, uint16_t num_pixels);
static void pattern_RunningLights(uint8_t red, uint8_t green, uint8_t blue, int WaveDelay, uint16_t num_pixels);
static void pattern_colorWipe(uint8_t red, uint8_t green, uint8_t blue, int SpeedDelay, uint16_t num_pixels);

//Create a massage structure for this module (pattern_generator)
static pixel_message_s TLS3001_data_packet;
//Allocate a local color array buffer to be used as storing the color data.
static uint16_t pattern_color_array[PIXELS_CONNECTED*3];

static uint16_t num_pixels_setting;

pattern_effect_enum pettern_effect = 0;

struct rgb_color_cmd_s {
	uint16_t red;
	uint16_t green;
	uint16_t blue;
} rgb_color;

int effect_delay;

void pattern_gen_task(void *arg) {	 
	while(1) {
		switch (pettern_effect) {
		case equal_color:
			pattern_equal_color(rgb_color.red, rgb_color.green, rgb_color.blue, num_pixels_setting);
			break;

		case Running_Lights:
			pattern_RunningLights((uint8_t) rgb_color.red, (uint8_t) rgb_color.green, (uint8_t) rgb_color.blue, effect_delay, num_pixels_setting);
			break;

		case colorWipe:
			pattern_colorWipe((uint8_t) rgb_color.red, (uint8_t) rgb_color.green, (uint8_t) rgb_color.blue, effect_delay, num_pixels_setting);
			pattern_colorWipe(0x00,0x00,0x00, effect_delay, num_pixels_setting);
			break;		 
		default:
			vTaskDelay(10 / portTICK_PERIOD_MS); 
			break;
		}
	}		
}

esp_err_t pattern_init() {
	TLS3001_data_packet.data_semaphore_guard = xSemaphoreCreateMutex();
	
	xTaskCreate(&pattern_gen_task, "pattern_gen_task", 4096, NULL, 5, NULL);

	ESP_LOGI(TAG, "pattern_gen initiated!");

	return ESP_OK;
}

static void delay(int delay) {
	vTaskDelay(((uint32_t)delay) / portTICK_PERIOD_MS);
}

static void pattern_TLS3001_show() {
	//Create a pointer to send over the queue
	//pixel_message_s *TLS3001_data_packet_p = &TLS3001_data_packet;

	TLS3001_send_to_queue(&TLS3001_data_packet, &pattern_color_array, num_pixels_setting);

	/*
	if( xSemaphoreTake(TLS3001_data_packet_p->data_semaphore_guard, ( TickType_t ) 10 ) == pdTRUE ) {
			//ESP_LOGD(TAG, "Generating equal color data");

			TLS3001_data_packet_p->color_data_p = &pattern_color_array;
			TLS3001_data_packet_p->pixel_len = num_pixels_setting;

			//Done with the data
			xSemaphoreGive(TLS3001_data_packet_p->data_semaphore_guard);

			//Send copy of pointer to pixel_message_s structure to TLS3001 task
			if(xQueueSend(TLS3001_input_queue, (void *) &TLS3001_data_packet_p,(TickType_t )10)) {
				//ESP_LOGD(TAG, "successfully posted pattern data on queue. Pixels: %d", TLS3001_data_packet_p->pixel_len);
			} else {
				ESP_LOGW(TAG, "Queue full. Did not post any data!");
			}

	} else {
		//The semaphore could not be taken. This would mean that the TLS3001_task is still processing the data.
		ESP_LOGW(TAG, "Semaphore busy! TLS3001_task still processing data");
	}
	*/
}

static void setPixel(int Pixel, uint8_t red, uint8_t green, uint8_t blue) {
	//Convert 8bit to 12bit
	float ratio = 4095.0f/255.0f;
	pattern_color_array[(Pixel*3)+0] = (uint16_t) red*ratio;
	pattern_color_array[(Pixel*3)+1] = (uint16_t) green*ratio;;
	pattern_color_array[(Pixel*3)+2] = (uint16_t) blue*ratio;;
}

static void pattern_equal_color(uint16_t red, uint16_t green, uint16_t blue, uint16_t num_pixels) {
	for (size_t i = 0; i < num_pixels; i++) {
		pattern_color_array[(i*3)+0] = red;
		pattern_color_array[(i*3)+1] = green;
		pattern_color_array[(i*3)+2] = blue;
	}

	pattern_TLS3001_show(num_pixels);
	delay(100);

	/*
	if ( xSemaphoreTake(pattern_data_packet_tp->data_semaphore_guard, ( TickType_t ) 10 ) == pdTRUE ) {
		ESP_LOGI(TAG, "Generating equal color data");

		pattern_data_packet_tp->color_data_p = &pattern_color_array;
		pattern_data_packet_tp->pixel_len = num_pixels;
		
		//Done with the data
		xSemaphoreGive(pattern_data_packet_tp->data_semaphore_guard);

		//Send copy of pointer to pixel_message_s structure to TLS3001 task
		if(xQueueSend(TLS3001_input_queue, (void *) &pattern_data_packet_tp,(TickType_t )10)) {
			ESP_LOGI(TAG, "successfully posted pattern data on queue");
		} else {
			ESP_LOGW(TAG, "Queue full. Did not post any data!");
		}		

	} else {
		//The semaphore could not be taken. This would mean that the TLS3001_task is still processing the data.
		ESP_LOGW(TAG, "Semaphore busy! TLS3001_task still processing data");
	}
	*/
}

static void pattern_RunningLights(uint8_t red, uint8_t green, uint8_t blue, int WaveDelay, uint16_t num_pixels) {
	int Position=0;

	for (int i=0; i<num_pixels*2; i++) {
		Position++; // = 0; //Position + Rate;
		for(int i=0; i<num_pixels; i++) {
			// sine wave, 3 offset waves make a rainbow!
			//float level = sin(i+Position) * 127 + 128;
			//setPixel(i,level,0,0);
			//float level = sin(i+Position) * 127 + 128;
			setPixel(i,((sin(i+Position) * 127 + 128)/255)*red,
						 ((sin(i+Position) * 127 + 128)/255)*green,
						 ((sin(i+Position) * 127 + 128)/255)*blue);
		}

		pattern_TLS3001_show(num_pixels);
		delay(WaveDelay);
	}
}

static void pattern_colorWipe(uint8_t red, uint8_t green, uint8_t blue, int SpeedDelay, uint16_t num_pixels) {
	for (uint16_t i=0; i<num_pixels; i++) {
		setPixel(i, red, green, blue);
		pattern_TLS3001_show(num_pixels);
		delay(SpeedDelay);
	}
}

/* 
void Fire(int Cooling, int Sparking, int SpeedDelay, uint16_t num_pixels) {
	static byte heat[num_pixels];
	int cooldown;
	
	// Step 1.	Cool down every cell a little
	for (int i = 0; i < num_pixels; i++) {
		cooldown = random(0, ((Cooling * 10) / num_pixels) + 2);
		
		if (cooldown>heat[i]) {
			heat[i] = 0;
		} else {
			heat[i] = heat[i]-cooldown;
		}
	}
	
	// Step 2.	Heat from each cell drifts 'up' and diffuses a little
	for (int k= num_pixels - 1; k >= 2; k--) {
		heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
	}
	
	// Step 3.	Randomly ignite new 'sparks' near the bottom
	if (random(255) < Sparking ) {
		int y = random(7);
		heat[y] = heat[y] + random(160, 255);
		//heat[y] = random(160,255);
	}

	// Step 4.	Convert heat to LED colors
	for( int j = 0; j < num_pixels; j++) {
		setPixelHeatColor(j, heat[j] );
	}

	showStrip();
	delay(SpeedDelay);
}
*/

void pattern_set_effect(pattern_effect_enum pattern_effect_cmd, uint16_t *rgb_cmd, int delay_cmd) {
	pettern_effect = pattern_effect_cmd;

	rgb_color.red = *rgb_cmd;
	rgb_color.green = *(rgb_cmd+1);
	rgb_color.blue = *(rgb_cmd+2);
	effect_delay = delay_cmd;
}

void pattern_set_pixel_number(uint16_t num_pixels) {
	//Todo: Put semaphore guard here?
	num_pixels_setting = num_pixels;
}

