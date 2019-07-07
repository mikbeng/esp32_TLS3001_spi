
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
#include "math.h"


void *spi_tx_data_start;

static esp_err_t init_spi_data_buffer(void **spi_manchester_data_p, uint32_t byte_len);
static void deinit_spi_data_buffer(void *spi_manchester_data_p);
static void *pack_manchester_data_segment(uint8_t *spi_mem_data_p_start, uint64_t data_in, uint32_t bit_length_manch, bool last_segment_flag);
static void TLS3001_prep_color_packet(uint8_t *spi_tx_data_start, uint16_t *color_data, TLS3001_handle_s *self);
static esp_err_t TLS3001_send_packet(void *data, uint32_t length, TLS3001_handle_s *self);
static esp_err_t TLS3001_send_color_packet(void *spi_tx_data_start, TLS3001_handle_s *self);
static esp_err_t TLS3001_send_resetsynch_packet(void *spi_tx_data_start, TLS3001_handle_s *self);

static const char * TAG = "TLS3001";

uint64_t reset_cmd;
uint64_t synch_cmd;
uint64_t start_cmd;

static void TLS3001_task(void *arg)
{
	TLS3001_handle_s *self = arg;
	uint16_t input_color_array[(self->config.num_pixels)*3];

	// Prepare the reset and synch commands
	pack_manchester_data_segment(&reset_cmd, RESET_CMD, RESET_CMD_LEN_MANCH, true);	
	pack_manchester_data_segment(&synch_cmd, SYNCH_CMD, SYNCH_CMD_LEN_MANCH, true);
	pack_manchester_data_segment(&start_cmd, START_CMD, START_CMD_LEN_MANCH, true);

	TLS3001_send_resetsynch_packet(spi_tx_data_start, self);
	
	uint16_t test_color[3] = {3000, 0, 1000};
	uint16_t test_pixels = 50;
	//pattern_equal_color(spi_tx_data_start, &test_color, self_config->num_pixels);

	//Send colors
	TLS3001_send_color_packet(spi_tx_data_start, self);

    while(1) {

		//Check incomming data queue
		xQueueReceive(self->data_in_queue, &input_color_array, 0);

		TLS3001_prep_color_packet(spi_tx_data_start, &input_color_array, self);

		//Send colors
		TLS3001_send_color_packet(spi_tx_data_start, self);

        vTaskDelay(100 / portTICK_PERIOD_MS);

    }
}

esp_err_t TLS3001_init(TLS3001_handle_s *TLS3001_handle)
{
	esp_err_t ret;
	uint16_t buffer_mem_size_byte;

	buffer_mem_size_byte = (uint16_t ) ceil(((TLS3001_handle->config.num_pixels*PIXEL_DATA_LEN_SPI)+START_CMD_LEN_SPI)/8);

	ret = init_spi_data_buffer(&spi_tx_data_start, buffer_mem_size_byte);
	if(ret != ESP_OK)
	{
		ESP_LOGE(__func__, "init_spi_data_buffer(): returned %d", ret);
		return ret;
	}

	xTaskCreate(&TLS3001_task, "TLS3001_task", configMINIMAL_STACK_SIZE, TLS3001_handle, 5, TLS3001_handle->TLS3001_task_handle);
	/*
	if (!(pdPASS == xTaskCreatePinnedToCore(&TLS3001_task, "TLS3001_task", 4096, config, configMAX_PRIORITIES - 1, NULL, LOOP_CORE)))
	{
		return ESP_FAIL;
	}*/

	ESP_LOGI(TAG, "TLS3001 initiated! Pixels: %d. Buffer memory size: %d", TLS3001_handle->config.num_pixels, buffer_mem_size_byte);

	return ESP_OK;
}

static esp_err_t init_spi_data_buffer(void **spi_manchester_data_p, uint32_t byte_len)
{
	*spi_manchester_data_p = heap_caps_malloc(byte_len, (MALLOC_CAP_DMA | MALLOC_CAP_32BIT));

	if(*spi_manchester_data_p == NULL)
	{
		return ESP_FAIL;
	}

	return ESP_OK;
}

static void deinit_spi_data_buffer(void *spi_manchester_data_p)
{
	heap_caps_free(spi_manchester_data_p);
}


static void TLS3001_prep_color_packet(uint8_t *spi_tx_data_start, uint16_t *color_data, TLS3001_handle_s *self)
{
	uint8_t *spi_tx_data_last;
	//Todo: make sure that pixel1_red starts with 0b0......
    //Color data will be on the form: [red,green,blue,red,green,blue,...]

	spi_tx_data_last = pack_manchester_data_segment(spi_tx_data_start, START_CMD, START_CMD_LEN_MANCH, false);

	if (self->config.num_pixels == 1)
	{
		spi_tx_data_last = pack_manchester_data_segment(spi_tx_data_start, (uint64_t)*(color_data+0 ), COLOR_DATA_LEN_MANCH, false);
	    spi_tx_data_last = pack_manchester_data_segment(spi_tx_data_start, (uint64_t)*(color_data+1 ), COLOR_DATA_LEN_MANCH, false);
	    spi_tx_data_last = pack_manchester_data_segment(spi_tx_data_start, (uint64_t)*(color_data+2 ), COLOR_DATA_LEN_MANCH, true);
	}
	else
	{
		//For loop for filling 1 color of data at a time.
		for (size_t i = 0; i < ((self->config.num_pixels*3)-1); i++)
		{
			spi_tx_data_last = pack_manchester_data_segment(spi_tx_data_start, (uint64_t)*(color_data+i), COLOR_DATA_LEN_MANCH, false);
		}

		spi_tx_data_last = pack_manchester_data_segment(spi_tx_data_start, (uint64_t)*(color_data+((self->config.num_pixels*3)-1) ), COLOR_DATA_LEN_MANCH, true);

	}
    

} 

//Generic function for packing manchester data bits to memory. Aligns correctly in memory with SPI transfers
static void *pack_manchester_data_segment(uint8_t *spi_mem_data_p_start, uint64_t data_in, uint32_t bit_length_manch, bool last_segment_flag)
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

static esp_err_t TLS3001_send_packet(void *data, uint32_t length, TLS3001_handle_s *self)
{
	esp_err_t ret;
	static spi_transaction_t t;

	memset(&t, 0, sizeof(t));        	//Zero out the transaction
	t.tx_buffer = data;
	t.length = length;
	t.rxlength = 0;
	
	ret = spi_device_polling_transmit(self->spi_handle, &t); 	//Transmit!
	if(ret != ESP_OK)
	{
		ESP_LOGW(__func__, "spi_device_transmit(): returned %d", ret);
		return ret;
	}
	
	return ESP_OK;
}

static esp_err_t TLS3001_send_color_packet(void *spi_tx_data_start, TLS3001_handle_s *self)
{
	//Function for sending start and colorpacket from memory pointed by arg spi_tx_data_start

	esp_err_t ret;
	ret = spi_device_acquire_bus(self->spi_handle, portMAX_DELAY);
	if(ret != ESP_OK)
	{
		ESP_LOGE(__func__, "spi_device_acquire_bus(): returned %d", ret);
		return ret;
	}

	ret = TLS3001_send_packet(spi_tx_data_start, ((self->config.num_pixels*PIXEL_DATA_LEN_SPI)+START_CMD_LEN_SPI), self);
	if(ret != ESP_OK)
	{
		ESP_LOGE(__func__, "TLS3001_send_packet(): returned %d", ret);
		return ret;
	}

	ets_delay_us(150); 

	ret = TLS3001_send_packet(&start_cmd, START_CMD_LEN_SPI, self);
	if(ret != ESP_OK)
	{
		ESP_LOGE(__func__, "TLS3001_send_packet(): returned %d", ret);
		return ret;
	}

	spi_device_release_bus(self->spi_handle);	

	return ESP_OK;

}

static esp_err_t TLS3001_send_resetsynch_packet(void *spi_tx_data_start, TLS3001_handle_s *self) 
{
	ESP_ERROR_CHECK(spi_device_acquire_bus(self->spi_handle, portMAX_DELAY));

	//Send RESET	
	ESP_ERROR_CHECK(TLS3001_send_packet(&reset_cmd, RESET_CMD_LEN_SPI,self));			
	
	//Delays 1ms
	ets_delay_us(1000); 										
	
	//Send SYNCH
	ESP_ERROR_CHECK(TLS3001_send_packet(&synch_cmd, SYNCH_CMD_LEN_SPI,self));

	spi_device_release_bus(self->spi_handle);

	ets_delay_us(SYNCH_DELAY_PER_PIXEL * self->config.num_pixels); 	//min delay of 28.34us times the number of pixels

	return ESP_OK;
}