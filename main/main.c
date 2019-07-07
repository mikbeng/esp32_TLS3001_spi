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

#define PIN_NUM_MOSI 5
#define PIN_NUM_CLK  18

esp_err_t SPI_init(TLS3001_handle_s *TLS3001_handle);
void post_cb_func(spi_transaction_t* trans);

void app_main()
{

	gpio_pad_select_gpio(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);


	spi_device_handle_t HSPI;
	QueueHandle_t  q1=NULL;

	q1=xQueueCreate(20,sizeof(unsigned long));
    if(q1 == NULL){
		ESP_LOGE(__func__, "xQueueCreate() failed");
    }

	TLS3001_handle_s TLS3001_handle_ch1 = {
		{
			.num_pixels = 50,
			.spi_channel = HSPI_HOST,
			.spi_freq = 500000,
		},
		.spi_handle = &HSPI,
		.data_in_queue = q1,
		.TLS3001_task_handle = NULL
	};

	if (SPI_init(&TLS3001_handle_ch1) != ESP_OK) {
		printf("Error initializing spi\n");
		while (1) {
			vTaskDelay(1000 / portTICK_PERIOD_MS);
		}
	}

	TLS3001_init(&TLS3001_handle_ch1);

}


esp_err_t SPI_init(TLS3001_handle_s *TLS3001_handle)
{
	esp_err_t ret;
	
	spi_bus_config_t buscfg = {
		.miso_io_num = -1,
		.mosi_io_num = PIN_NUM_MOSI,
		.sclk_io_num = PIN_NUM_CLK,
		.quadwp_io_num = -1,
		.quadhd_io_num = -1
	};
	
	//New config parameters for DMA
	spi_device_interface_config_t devcfg = {
		.command_bits = 0,
		.address_bits = 0,
		.dummy_bits = 0,
		.clock_speed_hz = TLS3001_handle->config.spi_freq, 
		 	//Clock out at 6 MHz				
		.mode = 1, 
							//SPI mode 1		                               
		.spics_io_num = -1,
		 			//-1 if manually toggle CS 			
		.queue_size = 1, 
					//We want to be able to queue 4 transactions at a time        
		.cs_ena_posttrans = 0, 
				//CS hold post trans in us
		.cs_ena_pretrans = 0, 
				//CS setup time in us
		.flags = (SPI_DEVICE_HALFDUPLEX) | (SPI_DEVICE_3WIRE),
		.input_delay_ns = 0	
		//.pre_cb = pre_cb_func,		//pre interrupt for manually toggle CS
	    //.post_cb = post_cb_func			//post interrupt for manually toggle CS
	};
		
	//Initialize the SPI bus with DMA channel 1
	ret = spi_bus_initialize(TLS3001_handle->config.spi_channel, &buscfg, 1);
	if (ret != ESP_OK)
	{
		ESP_LOGE(__func__, "spi_bus_initialize(): returned %d", ret);
	}
	assert(ret == ESP_OK);
	
	//Attach the device to the SPI bus
	ret = spi_bus_add_device(HSPI_HOST, &devcfg, TLS3001_handle->spi_handle);
	if (ret != ESP_OK)
	{
		ESP_LOGE(__func__, "spi_bus_add_device(): returned %d", ret);
	}
	assert(ret == ESP_OK);
	
	return ESP_OK;
}

void post_cb_func(spi_transaction_t* trans) {
		//Callback when a queued SPI transmit has completed
	
	//user_timer_delay_start();	//Maybe start timer in post_cb_func??

}