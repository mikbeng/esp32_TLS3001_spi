#pragma once

#include "driver/spi_master.h"

#define BLINK_GPIO CONFIG_BLINK_GPIO

extern uint64_t reset_cmd;
extern uint64_t synch_cmd;
extern uint64_t start_cmd;
extern spi_device_handle_t spi;