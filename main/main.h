#pragma once

#include "TLS3001.h"
#include "freertos/queue.h"

#define TEST_PIXELS 2

extern QueueHandle_t  TLS3001_input_queue;

#define BLINK_GPIO CONFIG_BLINK_GPIO
