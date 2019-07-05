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

#define RESET_CMD 0x7FFF4
#define RESET_CMD_LEN_MANCH (19)
#define RESET_CMD_LEN_SPI (RESET_CMD_LEN_MANCH * 2)

#define SYNCH_CMD 0x3FFF8800
#define SYNCH_CMD_LEN_MANCH (30)
#define SYNCH_CMD_LEN_SPI (SYNCH_CMD_LEN_MANCH * 2)

#define START_CMD 0x7FFF2
#define START_CMD_LEN_MANCH (19)
#define START_CMD_LEN_SPI (START_CMD_LEN_MANCH * 2)

#define COLOR_DATA_LEN_MANCH (13)
#define COLOR_DATA_LEN_SPI (COLOR_DATA_LEN_MANCH * 2)

#define PIXELS_NUMBER 100
#define RGB_PACKET_LEN_SPI (39*2)

#define SPI_SWAP_DATA_TX_64(data, len) __builtin_bswap64((uint64_t)data<<(64-len))
	
spi_device_handle_t spi;
void *spi_tx_data_start;

esp_err_t TLS3001_send_packet(void *data, uint32_t length);
void TLS3001_fill_packet_cmd(uint8_t *spi_manchester_data_p, uint64_t data_in, uint32_t bit_length);
esp_err_t SPI_init(void);
void init_spi_data_buffer(void **spi_manchester_data_p, uint32_t byte_len);
void deinit_spi_data_buffer(void *spi_manchester_data_p);
void post_cb_func(spi_transaction_t* trans);
void *TLS3001_pack_packet_color(uint8_t *spi_manchester_data_p, uint64_t data_in, uint32_t bit_length_manch, bool last_data_flag);
void *pack_manchester_data_segment(uint8_t *spi_mem_data_p_start, uint64_t data_in, uint32_t bit_length_manch, bool last_segment_flag);
void TLE3001_prep_color_data();
uint64_t prep_pixel_packet(uint64_t red, uint64_t green, uint64_t blue);
/* Can run 'make menuconfig' to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO CONFIG_BLINK_GPIO


uint64_t pixel_test = 0;

uint16_t pixel1_red = 4000;
uint16_t pixel1_green = 4;
uint16_t pixel1_blue = 16;



void blink_task(void *pvParameter)
{
	uint8_t *spi_tx_data_last;
	
	
	pixel_test = prep_pixel_packet(4000, 4, 16);
	
	//TLS3001_fill_packet_cmd(spi_tx_data, RESET_CMD, (RESET_CMD_LEN_BITS_SPI / 2));
	//TLS3001_send_packet(spi_tx_data, RESET_CMD_LEN_BITS_SPI);
	
	vTaskDelay(2 / portTICK_PERIOD_MS);
	
	//TLS3001_fill_packet_cmd(spi_tx_data, SYNCH_CMD, (SYNCH_CMD_LEN_BITS_SPI / 2));
	//TLS3001_send_packet(spi_tx_data, SYNCH_CMD_LEN_BITS_SPI);
	
	vTaskDelay(10 / portTICK_PERIOD_MS);
	
	//pointer spi_tx_data points to start.
	//spi_tx_data_last = TLS3001_pack_packet_color(spi_tx_data, START_CMD, (START_CMD_LEN_BITS_SPI / 2), false);
	//spi_tx_data_last = TLS3001_pack_packet_color(spi_tx_data_last, pixel_test, (RGB_PACKET_LEN_SPI / 2), true);

	//TLS3001_send_packet(spi_tx_data, (RGB_PACKET_LEN_SPI + START_CMD_LEN_BITS_SPI));
	
	
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
	    
	    //TLS3001_send_packet(spi_tx_data, RESET_CMD_LEN_BITS_SPI);
    }
}

void app_main()
{
	SPI_init();
	
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

void TLS3001_fill_packet_cmd(uint8_t *spi_manchester_data_p, uint64_t data_in, uint32_t bit_length_manch)
{
	static void *spi_manchester_data_last;
	void *spi_manchester_data_start;

	uint8_t byte_temp = 0;
	uint8_t byte_last_temp = 0;
	int32_t bits_remaining = bit_length_manch;

	spi_manchester_data_start = (void*)spi_manchester_data_p;
	
	while (bits_remaining > 0)
	{
		if (bits_remaining < 4)	//Bits remaining is somewhere between 1-3.
		{
			byte_temp = data_in;   //No need to right shift since we are already at lowest "byte"
			
			byte_last_temp |= (MANCHESTER_VALUE(byte_temp & (0x01 << (bits_remaining-1))) << 6);
			bits_remaining -= 1;
			if (bits_remaining <= 0)
			{
				*spi_manchester_data_p = (uint8_t)byte_last_temp;
				break;
			}
			
			byte_last_temp |= (MANCHESTER_VALUE(byte_temp & (0x01 << (bits_remaining - 1))) << 4);
			bits_remaining -= 1;
			if (bits_remaining <= 0)
			{
				*spi_manchester_data_p = (uint8_t)byte_last_temp;
				break;
			}
			
			byte_last_temp |= (MANCHESTER_VALUE(byte_temp & (0x01 << (bits_remaining - 1))) << 2);
			bits_remaining -= 1;
			if (bits_remaining <= 0)
			{
				*spi_manchester_data_p = (uint8_t)byte_last_temp;
				break;
			}
			
			break;	//Should be nothing left here. Last 
			
		}
		else
		{
			byte_temp = data_in >> (bits_remaining - 4);
			*spi_manchester_data_p = (uint8_t)(
				(MANCHESTER_VALUE(byte_temp & (0x08)) << 6) 
				| (MANCHESTER_VALUE(byte_temp & (0x04)) << 4) 
				| (MANCHESTER_VALUE(byte_temp & (0x02)) << 2) 
				| (MANCHESTER_VALUE(byte_temp & (0x01)) << 0));
		
			
			bits_remaining -= 4; 
		}
		
		spi_manchester_data_p++;

	}
	
	spi_manchester_data_last = (void*) spi_manchester_data_p;
	
	spi_manchester_data_p = (void*) spi_manchester_data_start;	//Point to start of buffer again on exit
	//return spi_manchester_data_start;
}

void TLE3001_prep_color_data()
{
	uint8_t *spi_tx_data_last;

	//Todo: make sure that pixel1_red starts with 0b0......

	spi_tx_data_last = pack_manchester_data_segment(spi_tx_data_start, START_CMD, START_CMD_LEN_MANCH, false);	
	spi_tx_data_last = pack_manchester_data_segment(spi_tx_data_start, pixel1_red, COLOR_DATA_LEN_MANCH, false);
	spi_tx_data_last = pack_manchester_data_segment(spi_tx_data_start, pixel1_green, COLOR_DATA_LEN_MANCH, false);
	spi_tx_data_last = pack_manchester_data_segment(spi_tx_data_start, pixel1_blue, COLOR_DATA_LEN_MANCH, false);

} 

//Generic function for packing manchester data bits to memory. Aligns correctly in memory with SPI transfers
void *pack_manchester_data_segment(uint8_t *spi_mem_data_p_start, uint64_t data_in, uint32_t bit_length_manch, bool last_segment_flag)
{
	
	static uint8_t byte_temp = 0;
	static uint8_t byte_pointer = 6;	
	static uint32_t mem_p_offset = 0;
	uint8_t *spi_mem_data_p_last = spi_mem_data_p_start + mem_p_offset;

	for (size_t i = 0; i < bit_length_manch; i++)
	{
		//byte_pointer = (6 - ((i % 4)*2));	//byte_pointer = 6,4,2,0,6,4,2,0, ....
		byte_temp |= (MANCHESTER_VALUE( (uint8_t)((data_in >> (bit_length_manch - i -1)) & 0x01) ) << byte_pointer);
		
		//i=15 (bit 3) bytepointer = 0
		//i=16 (bit 2) bytepointer = 6
		//i=17 (bit 1) bytepointer = 4
		//i=18 (bit 0) bytepointer = 2

		if (byte_pointer == 0)	//byte full. Write byte to memory
		{
			*(spi_mem_data_p_start+mem_p_offset) = byte_temp;
			spi_mem_data_p_last = spi_mem_data_p_start+mem_p_offset;
			mem_p_offset ++;	//Offset-pointer variable points to next free memory

			byte_temp = 0;	//reset temporary byte 
			byte_pointer = 6;
		}

		byte_pointer = byte_pointer - 2;
	}

	if((byte_pointer != 0) && (last_segment_flag == true))
	{
		//We have remaining manchester data that didnt make it to one entire byte

		//Write last remaining data. Empty bits will be zero.
		*(spi_mem_data_p_start+mem_p_offset) = byte_temp;
		spi_mem_data_p_last = spi_mem_data_p_start+mem_p_offset;

		byte_temp = 0;	//reset temporary byte 
		byte_pointer = 6;
		mem_p_offset = 0;	//reset pointer offset
	
	}

	//Return pointer to last data.
	return spi_mem_data_p_last;	
}

void *TLS3001_pack_packet_color(uint8_t *spi_manchester_data_p, uint64_t data_in, uint32_t bit_length_manch, bool last_data_flag)
{
	uint8_t byte_temp = 0;
	static uint64_t data_leftover = 0;
	static uint32_t bits_leftover = 0;
	uint8_t byte_last_temp = 0;
	int32_t bits_remaining = 0;
	uint64_t data = 0;
	
	uint8_t *spi_manchester_data_p_last = spi_manchester_data_p;
	
	bits_remaining = bit_length_manch + bits_leftover;
	data = (uint64_t)(((data_leftover << (bit_length_manch + 1)) | data_in));
	
	
	while (bits_remaining > 0)
	{
		if (bits_remaining < 4)	//Bits remaining is somewhere between 1-3.
			{
				data_leftover = (uint8_t)(data & 0x0F);     
				bits_leftover = bits_remaining;
				
				if (last_data_flag == true)
				{
					byte_last_temp |= (MANCHESTER_VALUE(data_leftover & (0x01 << (bits_remaining - 1))) << 6);
					bits_remaining -= 1;
					if (bits_remaining <= 0)
					{
						*spi_manchester_data_p_last = (uint8_t)byte_last_temp;
						spi_manchester_data_p_last++;
						break;
					}
			
					byte_last_temp |= (MANCHESTER_VALUE(data_leftover & (0x01 << (bits_remaining - 1))) << 4);
					bits_remaining -= 1;
					if (bits_remaining <= 0)
					{
						*spi_manchester_data_p_last = (uint8_t)byte_last_temp;
						spi_manchester_data_p_last++;
						break;
					}
			
					byte_last_temp |= (MANCHESTER_VALUE(data_leftover & (0x01 << (bits_remaining - 1))) << 2);
					bits_remaining -= 1;
					if (bits_remaining <= 0)
					{
						*spi_manchester_data_p_last = (uint8_t)byte_last_temp;
						spi_manchester_data_p_last++;
						break;
					}
					
					break;	//Should be nothing left here. Last 
				}
				
				else
				{
					break;		//Exit loop and wait for new data.
				}
				
			}
		else
		{
			byte_temp = data >> (bits_remaining - 4);
			*spi_manchester_data_p_last = (uint8_t)(
				(MANCHESTER_VALUE(byte_temp & (0x08)) << 6) 
				| (MANCHESTER_VALUE(byte_temp & (0x04)) << 4) 
				| (MANCHESTER_VALUE(byte_temp & (0x02)) << 2) 
				| (MANCHESTER_VALUE(byte_temp & (0x01)) << 0));
		
			
			bits_remaining -= 4; 
			spi_manchester_data_p_last++;
		}
			

	}
	return spi_manchester_data_p_last;
}

uint64_t prep_pixel_packet(uint64_t red, uint64_t green, uint64_t blue)
{
	//Max number 4095 per color(12 bits) check for this
	uint64_t pixel_packet = 0;
	
	pixel_packet = (uint64_t)((red << (13 * 2)) | (green << (13 * 1)) | (blue << (13 * 0))); 
	
	return pixel_packet;
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

void post_cb_func(spi_transaction_t* trans) {
		//Callback when a queued SPI transmit has completed
	
	

}
