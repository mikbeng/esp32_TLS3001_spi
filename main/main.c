/* TLS3001 SPI

	Control interface for TLS3001 LED driver.
	Controllable via UART and sACN (Streaming ACN, E1.31, aka. DMX512 over WiFi)
*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
//#include "esp_heap_alloc_caps.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "driver/spi_master.h"
#include "rom/ets_sys.h"
#include "nvs_flash.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "sdkconfig.h"
#include "main.h"
#include "CLI_comp/CLI.h"
#include "TLS3001.h"
#include "pattern_generator.h"
#include "e131.h"
#include "settings.h"

static const char *TAG = "TLS3001";

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;

static esp_err_t event_handler(void *ctx, system_event_t *event) {
	switch (event->event_id) {
	case SYSTEM_EVENT_STA_START:
		esp_wifi_connect();
		break;
	case SYSTEM_EVENT_STA_GOT_IP:
		xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
		/* This is a workaround as ESP32 WiFi libs don't currently
		   auto-reassociate. */
		esp_wifi_connect();
		xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
		break;
	default:
		break;
	}
	return ESP_OK;
}

// static void initialise_wifi(void) {
// 	tcpip_adapter_init();
// 	wifi_event_group = xEventGroupCreate();
// 	ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
// 	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
// 	ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
// 	ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );

//     wifi_config_t wifi_config;
//     strcpy((const char *) &wifi_config.sta.ssid, &settings.ssid);
//     strcpy((const char *) &wifi_config.sta.password, &settings.password);

// 	ESP_LOGI(TAG, "Setting WiFi configuration SSID: '%s'", wifi_config.sta.ssid);
// 	ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
// 	ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
// 	ESP_ERROR_CHECK( esp_wifi_start() );
// }

void app_main()
{
	// Initialize NVS
	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
	}
	ESP_ERROR_CHECK( err );
	LoadSettings();

	uint16_t num_pixels_ch1 = 13;		//Maximum number of pixels on strip. 361 for the one on my desk.
    uint16_t num_pixels_ch2 = 13;	

	TLS3001_init(num_pixels_ch1, num_pixels_ch2);
	pattern_init();

	start_cli_passive_mode();	//Starts the CLI task

	//initialise_wifi();
	//xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
	//e131init();
}

