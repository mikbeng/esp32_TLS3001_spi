/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "stdint.h"
#include "stdbool.h"
#include <string.h>
#include "esp_heap_alloc_caps.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "main.h"
#include "TLS3001.h"
#include "rom/ets_sys.h"
#include "pattern_generator.h"
#include "CLI_comp/CLI.h"

void app_main()
{

	uint16_t num_pixels_user = PIXELS_CONNECTED;		//Maximum number of pixels on strip. 361 for the one on my desk.

	TLS3001_ch1_init(num_pixels_user);
	pattern_init();

	start_cli_passive_mode();	//Starts the CLI task

}