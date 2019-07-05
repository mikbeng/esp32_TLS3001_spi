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
#include "timer.h"

#define PIN_NUM_MOSI 5
#define PIN_NUM_CLK  18


esp_err_t SPI_init(void);
void init_spi_data_buffer(void **spi_manchester_data_p, uint32_t byte_len);
void deinit_spi_data_buffer(void *spi_manchester_data_p);
void post_cb_func(spi_transaction_t* trans);


spi_device_handle_t spi;

uint64_t reset_cmd;
uint64_t synch_cmd;
uint64_t start_cmd;
void *spi_tx_data_start;

uint64_t pixel_test = 0;
uint16_t pixel1_red = 4000;
uint16_t pixel1_green = 4;
uint16_t pixel1_blue = 16;

uint16_t color_array[3];

void blink_task(void *pvParameter)
{
	bool blink = false;
	uint8_t *spi_tx_data_last;
	
	color_array[0] = pixel1_red;
	color_array[1] = pixel1_green;
	color_array[2] = pixel1_blue;
	uint16_t num_pixels = 1;

	gpio_set_level(BLINK_GPIO, 0);

	// Prepare the reset and synch commands
	pack_manchester_data_segment(&reset_cmd, RESET_CMD, RESET_CMD_LEN_MANCH, true);	
	pack_manchester_data_segment(&synch_cmd, SYNCH_CMD, SYNCH_CMD_LEN_MANCH, true);

	//prepare the color data
	TLE3001_prep_color_packet(spi_tx_data_start, &color_array, num_pixels);

	TLS3001_send_packet(&reset_cmd, RESET_CMD_LEN_SPI);
	
	//Delay
	user_timer_start(1000);
	while(xSemaphoreTake( xSemaphore_delay, 0 ) != pdTRUE){
	}
	
	//vTaskDelay(2 / portTICK_PERIOD_MS);					//Need better delay here..
	TLS3001_send_packet(&synch_cmd, SYNCH_CMD_LEN_SPI);

	vTaskDelay(10 / portTICK_PERIOD_MS);				//Need better delay here..

	TLS3001_send_packet(spi_tx_data_start, (PIXEL_DATA_LEN_SPI+START_CMD_LEN_SPI));
	
	
    gpio_pad_select_gpio(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    while(1) {
        /* Blink off (output low) */
        //gpio_set_level(BLINK_GPIO, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        /* Blink on (output high) */
        //gpio_set_level(BLINK_GPIO, 1);
       
	/*
	if( xSemaphoreTake( xSemaphore_delay, 0xffff ) == pdTRUE )
       {
		    if(blink == false)
			{
				gpio_set_level(BLINK_GPIO, 1);
				blink = true;
			}
			else
			{
				gpio_set_level(BLINK_GPIO, 0);
				blink = false;
			}
           // It is time to execute.

           // ...

           // We have finished our task.  Return to the top of the loop where
           // we will block on the semaphore until it is time to execute
           // again.  Note when using the semaphore for synchronisation with an
           // ISR in this manner there is no need to 'give' the semaphore back.
       }
	    */
	    //TLS3001_send_packet(spi_tx_data, RESET_CMD_LEN_BITS_SPI);
    }
}

void app_main()
{
	if (SPI_init() != ESP_OK) {
		printf("Error initializing spi\n");
		while (1) {
			vTaskDelay(1000 / portTICK_PERIOD_MS);
		}
	}
	
	init_timer();

	init_spi_data_buffer(&spi_tx_data_start, 1500);	//1500 bytes for 100 pixels should be enough.
	
    xTaskCreate(&blink_task, "blink_task", configMINIMAL_STACK_SIZE, NULL, 5, NULL);
}

void init_spi_data_buffer(void **spi_manchester_data_p, uint32_t byte_len)
{
	*spi_manchester_data_p = heap_caps_malloc(byte_len, MALLOC_CAP_8BIT);
}

void deinit_spi_data_buffer(void *spi_manchester_data_p)
{
	heap_caps_free(spi_manchester_data_p);
}

esp_err_t SPI_init(void)
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
		.clock_speed_hz = (500000), 
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
	    //.post_cb = post_cb_func,		//post interrupt for manually toggle CS
	};
		
	
	/*For DMA*/
	//dma_motion_data_p = heap_caps_malloc(64,
	//	(MALLOC_CAP_DMA | MALLOC_CAP_32BIT));
	
	//spi_rxdata_p = heap_caps_malloc(64, MALLOC_CAP_DEFAULT);         	//For no DMA

	//Initialize the SPI bus
	ret = spi_bus_initialize(HSPI_HOST, &buscfg, 0);
	if (ret != ESP_OK)
	{
		ESP_LOGE(__func__, "spi_bus_initialize(): returned %d", ret);
	}
	assert(ret == ESP_OK);
	
	//Attach the device to the SPI bus
	ret = spi_bus_add_device(HSPI_HOST, &devcfg, &spi);
	if (ret != ESP_OK)
	{
		ESP_LOGE(__func__, "spi_bus_add_device(): returned %d", ret);
	}
	assert(ret == ESP_OK);

	/*
	spi_max_freq = spi_get_freq_limit(true, devcfg.input_delay_ns);
	ESP_LOGI(__func__, "SPI max freq: %d", spi_max_freq);
	*/
	
	spi_device_acquire_bus(spi, portMAX_DELAY);
	
	return ESP_OK;
}

void post_cb_func(spi_transaction_t* trans) {
		//Callback when a queued SPI transmit has completed
	
	

}
