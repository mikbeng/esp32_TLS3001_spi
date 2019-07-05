
#include "TLS3001.h"
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
#include "main.h"


void TLE3001_prep_color_packet(uint8_t *spi_tx_data_start, uint16_t *color_data, uint16_t num_pixels)
{
	uint8_t *spi_tx_data_last;
	//Todo: make sure that pixel1_red starts with 0b0......
    //Color data will be on the form: [red,green,blue,red,green,blue,...]

	spi_tx_data_last = pack_manchester_data_segment(spi_tx_data_start, START_CMD, START_CMD_LEN_MANCH, false);

	if (num_pixels == 1)
	{
		spi_tx_data_last = pack_manchester_data_segment(spi_tx_data_start, (uint64_t)*(color_data+0 ), COLOR_DATA_LEN_MANCH, false);
	    spi_tx_data_last = pack_manchester_data_segment(spi_tx_data_start, (uint64_t)*(color_data+1 ), COLOR_DATA_LEN_MANCH, false);
	    spi_tx_data_last = pack_manchester_data_segment(spi_tx_data_start, (uint64_t)*(color_data+2 ), COLOR_DATA_LEN_MANCH, true);
	}
	else
	{
		//For loop for filling 1 color of data at a time.
		for (size_t i = 0; i < ((num_pixels*3)-1); i++)
		{
			spi_tx_data_last = pack_manchester_data_segment(spi_tx_data_start, (uint64_t)*(color_data+i), COLOR_DATA_LEN_MANCH, false);
		}

		spi_tx_data_last = pack_manchester_data_segment(spi_tx_data_start, (uint64_t)*(color_data+((num_pixels*3)-1) ), COLOR_DATA_LEN_MANCH, true);

	}
    

} 

//Generic function for packing manchester data bits to memory. Aligns correctly in memory with SPI transfers
void *pack_manchester_data_segment(uint8_t *spi_mem_data_p_start, uint64_t data_in, uint32_t bit_length_manch, bool last_segment_flag)
{
	
	static uint8_t byte_temp = 0;
	static uint8_t byte_pointer = 8;	
	static uint32_t mem_p_offset = 0;
	uint8_t *spi_mem_data_p_last = spi_mem_data_p_start + mem_p_offset;

	for (size_t i = 0; i < bit_length_manch; i++)
	{
		byte_pointer = byte_pointer - 2;
		byte_temp |= (MANCHESTER_VALUE( (uint8_t)((data_in >> (bit_length_manch - i -1)) & 0x01) ) << byte_pointer);
		
		if (byte_pointer == 0)	//byte full. Write byte to memory
		{
			*(spi_mem_data_p_start+mem_p_offset) = byte_temp;
			spi_mem_data_p_last = spi_mem_data_p_start+mem_p_offset;
			mem_p_offset ++;	//Offset-pointer variable points to next free memory

			byte_temp = 0;	//reset temporary byte 
			byte_pointer = 8;
		}
			
	}

	if((byte_pointer != 0) && (last_segment_flag == true))
	{
		//We have remaining manchester data that didnt make it to one entire byte

		//Write last remaining data. Empty bits will be zero.
		*(spi_mem_data_p_start+mem_p_offset) = byte_temp;
		spi_mem_data_p_last = spi_mem_data_p_start+mem_p_offset;

		byte_temp = 0;	//reset temporary byte 
		byte_pointer = 8;
		mem_p_offset = 0;	//reset pointer offset
	
	}

	//Return pointer to last data.
	return spi_mem_data_p_last;	
}

esp_err_t TLS3001_send_packet(void *data, uint32_t length)
{
	esp_err_t ret;
	static spi_transaction_t t;

	memset(&t, 0, sizeof(t));        	//Zero out the transaction
	t.tx_buffer = data;
	t.length = length;
	t.rxlength = 0;
	
	ret = spi_device_polling_transmit(spi, &t); 	//Transmit!
	if(ret != ESP_OK)
	{
		ESP_LOGW(__func__, "spi_device_transmit(): returned %d", ret);
		return ret;
	}
	
	return ESP_OK;
}