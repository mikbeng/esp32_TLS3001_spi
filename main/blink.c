/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "stdint.h"
#include "stdbool.h"
#include <string.h>
#include "esp_heap_alloc_caps.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "sdkconfig.h"

#define PIN_NUM_MOSI 5
#define PIN_NUM_CLK  18

#define MANCHESTER_ONE 0x02		//0b10
#define MANCHESTER_ZERO 0x01	//0b01

#define MANCHESTER_VALUE(x)  ((x) ? MANCHESTER_ONE : MANCHESTER_ZERO)

#define MANCHESTER_UPPER_BLOCK(a) \
    (uint64_t) (   \
	    MANCHESTER_VALUE(a & 0x04) << 36 \
		| MANCHESTER_VALUE(a & 0x02) << 34 \
		| MANCHESTER_VALUE(a & 0x01) << 32 \
    )

#define MANCHESTER_MIDDLE_BLOCK(b) \
    (uint64_t) (   \
          MANCHESTER_VALUE(b & 0x80) << 30 \
        | MANCHESTER_VALUE(b & 0x40) << 28 \
        | MANCHESTER_VALUE(b & 0x20) << 26 \
        | MANCHESTER_VALUE(b & 0x10) << 24 \
        | MANCHESTER_VALUE(b & 0x08) << 22 \
        | MANCHESTER_VALUE(b & 0x04) << 20 \
        | MANCHESTER_VALUE(b & 0x02) << 18 \
        | MANCHESTER_VALUE(b & 0x01) << 16 \
    )
#define MANCHESTER_BYTE_BLOCK1(c) \
    (uint64_t) (    \
          MANCHESTER_VALUE(c & 0x80) << 14 \
        | MANCHESTER_VALUE(c & 0x40) << 12 \
        | MANCHESTER_VALUE(c & 0x20) << 10 \
        | MANCHESTER_VALUE(c & 0x10) << 8 \
        | MANCHESTER_VALUE(c & 0x08) << 6 \
        | MANCHESTER_VALUE(c & 0x04) << 4 \
        | MANCHESTER_VALUE(c & 0x02) << 2 \
        | MANCHESTER_VALUE(c & 0x01) \
    )

	
#define MANCHESTER_BLOCK(a, b, c) \
    ((uint64_t) (MANCHESTER_UPPER_BLOCK(a) | MANCHESTER_MIDDLE_BLOCK(a) | MANCHESTER_LOWER_BLOCK(b) ))

#define SPI_SWAP_DATA_TX_64(data, len) __builtin_bswap64((uint64_t)data<<(64-len))
	
spi_device_handle_t spi;
uint32_t *spi_txdata_p;

esp_err_t TLS3001_send_packet(void *data, uint32_t length);
void *TLS3001_fill_packet(uint64_t data_in, uint32_t bit_length);
esp_err_t SPI_init(void);


uint64_t reset_cmd = 0;
uint64_t start_cmd = 0;
uint64_t swapped_data;

void *spi_tx_data;

/* Can run 'make menuconfig' to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO CONFIG_BLINK_GPIO

void blink_task(void *pvParameter)
{
    /* Configure the IOMUX register for pad BLINK_GPIO (some pads are
       muxed to GPIO on reset already, but some default to other
       functions and need to be switched to GPIO. Consult the
       Technical Reference for a list of pads and their default
       functions.)
    */
    gpio_pad_select_gpio(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    while(1) {
        /* Blink off (output low) */
        gpio_set_level(BLINK_GPIO, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        /* Blink on (output high) */
        gpio_set_level(BLINK_GPIO, 1);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
	    
	    TLS3001_send_packet(spi_tx_data, 38);
    }
}

void app_main()
{
	//reset_cmd = 0x2AAAAAAA65;
	reset_cmd = 0x7FFF4;
	swapped_data = SPI_SWAP_DATA_TX_64(reset_cmd, 40);
	
	spi_tx_data = TLS3001_fill_packet(reset_cmd, 19);
	
	/*test_reset[1] = (uint64_t)(
        MANCHESTER_VALUE(1) << 30
        | MANCHESTER_VALUE(0) << 28
        | MANCHESTER_VALUE(0) << 26
    );*/
	SPI_init();
	
    xTaskCreate(&blink_task, "blink_task", configMINIMAL_STACK_SIZE, NULL, 5, NULL);
}

void *TLS3001_fill_packet(uint64_t data_in, uint32_t bit_length)
{
	static void *spi_manchester_data_last;
	void *spi_manchester_data_start;
	uint8_t *spi_manchester_data_p;
	uint8_t byte_temp = 0;
	int32_t bits_remaining = bit_length;
	
	spi_manchester_data_start = heap_caps_malloc(bit_length * 8, MALLOC_CAP_8BIT);
	
	spi_manchester_data_p = spi_manchester_data_start;
	
	while (bits_remaining > 0)
	{
		byte_temp = data_in >> (bits_remaining - 4);
		*spi_manchester_data_p = (uint8_t)(
			(MANCHESTER_VALUE(byte_temp & (0x08)) << 6) 
			| (MANCHESTER_VALUE(byte_temp & (0x04)) << 4) 
			| (MANCHESTER_VALUE(byte_temp & (0x02)) << 2) 
			| (MANCHESTER_VALUE(byte_temp & (0x01)) << 0));
		
		spi_manchester_data_p++;
		bits_remaining -= 4; 
	}
	
	spi_manchester_data_last = (void*) spi_manchester_data_p;
	
	return spi_manchester_data_start;
}

esp_err_t TLS3001_send_packet(void *data, uint32_t length)
{
	esp_err_t ret;
	static spi_transaction_t t;
	
	//uint64_t pixel_color = 0x

	memset(&t, 0, sizeof(t));        	//Zero out the transaction
	t.tx_buffer = data;
	t.length = length;
	t.rxlength = 0;
	
	ret = spi_device_transmit(spi, &t); 	//Transmit!
	if(ret != ESP_OK)
	{
		ESP_LOGW(__func__, "spi_device_transmit(): returned %d", ret);
		return ret;
	}
	
	return ESP_OK;
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