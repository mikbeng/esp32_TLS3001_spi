/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "stdint.h"
#include "stdbool.h"
#include <string.h>
#include "esp_heap_alloc_caps.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "main.h"
#include "TLS3001.h"
#include "rom/ets_sys.h"
#include "pattern_generator.h"

QueueHandle_t  TLS3001_input_queue;

//#define configUSE_TASK_NOTIFICATIONS 1

void post_cb_func(spi_transaction_t* trans);

void app_main()
{

	gpio_pad_select_gpio(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
	gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

	uint16_t num_pixels_user = TEST_PIXELS;

	TLS3001_input_queue=xQueueCreate(1,sizeof(pixel_message_s));
    if(TLS3001_input_queue == NULL){
		ESP_LOGE(__func__, "xQueueCreate() failed");
    }

/*
	volatile TLS3001_handle_s TLS3001_handle_ch1 = {
		.spi_tx_data_start = NULL,
		.num_pixels = num_pixels_user,
		.spi_channel = HSPI_HOST,
		.spi_freq = 500000,
		.spi_handle = NULL
	};
	
	volatile TLS3001_Ctrl_s TLS3001_Ctrl = {
		.TLS3001_ch1 = &TLS3001_handle_ch1,
		.TLS3001_ch2 = NULL,
		.TLS3001_data_in_queue = q1,
		.TLS3001_task_handle = NULL
	};

	TLS3001_init(&TLS3001_handle_ch1);

	pattern_init(&TLS3001_Ctrl);

	xTaskCreate(&TLS3001_task, "TLS3001_task", 4096, &TLS3001_Ctrl, configMAX_PRIORITIES - 1, &TLS3001_Ctrl.TLS3001_task_handle);

	xTaskCreate(&pattern_gen_task, "pattern_gen_task", 4096, &TLS3001_Ctrl, 5, NULL);
*/	

	TLS3001_ch1_init(num_pixels_user);
	pattern_init(num_pixels_user);

	/*
	if (!(pdPASS == xTaskCreatePinnedToCore(&TLS3001_task, "TLS3001_task", 4096, config, configMAX_PRIORITIES - 1, NULL, LOOP_CORE)))
	{
		return ESP_FAIL;
	}*/

}



/*
void post_cb_func(spi_transaction_t* trans) {
		//Callback when a queued SPI transmit has completed
	
	//user_timer_delay_start();	//Maybe start timer in post_cb_func??

}*/