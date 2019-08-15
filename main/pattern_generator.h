#pragma once

#include "stdint.h"
#include "main.h"

esp_err_t pattern_init(uint16_t num_pixels);
void pattern_send_equal_color(uint16_t *rgb, uint16_t num_pixels);

extern SemaphoreHandle_t patten_data_semaphore;