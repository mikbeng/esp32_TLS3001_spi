#pragma once

#include "esp_err.h"
#include "stdbool.h"
#include "driver/spi_master.h"
#include "freertos/task.h"

#define num_pixels_max 50

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

#define PIXEL_DATA_LEN_MANCH (39)
#define PIXEL_DATA_LEN_SPI (PIXEL_DATA_LEN_MANCH * 2)

#define RGB_PACKET_LEN_SPI (39*2)

#define SYNCH_DELAY_PER_PIXEL (28.34)

#define LOOP_CORE 1

typedef struct
{
    uint16_t num_pixels;
    spi_host_device_t spi_channel;
    uint16_t spi_freq;
	
}TLS3001_config_s;

typedef struct
{
    TLS3001_config_s config;
    spi_device_handle_t *spi_handle;
    QueueHandle_t  data_in_queue;	
    TaskHandle_t *TLS3001_task_handle;
}TLS3001_handle_s;

esp_err_t TLS3001_init(TLS3001_handle_s *TLS3001_handle);
