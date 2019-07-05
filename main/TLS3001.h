#pragma once

#include "esp_err.h"
#include "stdbool.h"

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

#define SYNCH_DELAY (28.34*num_pixels_max)

void *pack_manchester_data_segment(uint8_t *spi_mem_data_p_start, uint64_t data_in, uint32_t bit_length_manch, bool last_segment_flag);
void TLE3001_prep_color_packet(uint8_t *spi_tx_data_start, uint16_t *color_data, uint16_t num_pixels);
esp_err_t TLS3001_send_packet(void *data, uint32_t length);
esp_err_t TLS3001_send_color_packet(void *spi_tx_data_start, uint16_t num_pixels);
