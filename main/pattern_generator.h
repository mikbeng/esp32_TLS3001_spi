#pragma once

#include "stdint.h"
#include "main.h"

esp_err_t pattern_init(uint16_t num_pixels);

extern SemaphoreHandle_t patten_data_semaphore;