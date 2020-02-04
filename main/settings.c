#include "settings.h"

#include <stdint.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

static const char *TAG = "Settings";

void LoadSettings() {
	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		// NVS partition was truncated and needs to be erased
		// Retry nvs_flash_init
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
	}
	ESP_ERROR_CHECK(err);

	nvs_handle load_handle;
	size_t str_len;

	err = nvs_open("storage", NVS_READWRITE, &load_handle);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Error %s opening NVS handle", esp_err_to_name(err));
	} else {
		err = nvs_get_u16(load_handle, "universeStart", &settings.universeStart);
		switch (err) {
			case ESP_OK:
				break;
			case ESP_ERR_NVS_NOT_FOUND:
				settings.universeStart = 1;
				ESP_LOGI(TAG, "The universeStart is not initialized yet. Setting to default value: %d", settings.universeStart);
				break;
			default:
				ESP_LOGE(TAG, "Error %s reading universeStart", esp_err_to_name(err));
		}

		err = nvs_get_u16(load_handle, "universeStop", &settings.universeStop);
		switch (err) {
			case ESP_OK:
				break;
			case ESP_ERR_NVS_NOT_FOUND:
				settings.universeStop = 2;
				ESP_LOGI(TAG, "The universeStop is not initialized yet. Setting to default value: %d", settings.universeStop);
				break;
			default:
				ESP_LOGE(TAG, "Error %s reading universeStop", esp_err_to_name(err));
		}

		err = nvs_get_u16(load_handle, "dmx_start", &settings.dmx_start);
		switch (err) {
			case ESP_OK:
				break;
			case ESP_ERR_NVS_NOT_FOUND:
				settings.dmx_start = 1;
				ESP_LOGI(TAG, "The dmx_start is not initialized yet. Setting to default value: %d", settings.dmx_start);
				break;
			default:
				ESP_LOGE(TAG, "Error %s reading dmx_start", esp_err_to_name(err));
		}

		err = nvs_get_u16(load_handle, "pixel_number", &settings.pixel_number);
		switch (err) {
			case ESP_OK:
				break;
			case ESP_ERR_NVS_NOT_FOUND:
				settings.pixel_number = 10;
				ESP_LOGI(TAG, "The pixel_number is not initialized yet. Setting to default value: %d", settings.pixel_number);
				break;
			default:
				ESP_LOGE(TAG, "Error %s reading pixel_number", esp_err_to_name(err));
		}

		err = nvs_get_u16(load_handle, "selected_pattern", &settings.selected_pattern);
		switch (err) {
			case ESP_OK:
				break;
			case ESP_ERR_NVS_NOT_FOUND:
				settings.selected_pattern = 0;
				ESP_LOGI(TAG, "The selected_pattern is not initialized yet. Setting to default value: %d", settings.selected_pattern);
				break;
			default:
				ESP_LOGE(TAG, "Error %s reading selected_pattern", esp_err_to_name(err));
		}

		str_len = sizeof(settings.ssid);
		err = nvs_get_str(load_handle, "ssid", &settings.ssid, &str_len);
		switch (err) {
			case ESP_OK:
				break;
			case ESP_ERR_NVS_NOT_FOUND:
				settings.ssid[0] = '\0';
				ESP_LOGI(TAG, "The ssid is not initialized yet. Setting first byte to null");
				break;
			default:
				ESP_LOGE(TAG, "Error %s reading ssid", esp_err_to_name(err));
		}

		str_len = sizeof(settings.password);
		err = nvs_get_str(load_handle, "password", &settings.password, &str_len);
		switch (err) {
			case ESP_OK:
				break;
			case ESP_ERR_NVS_NOT_FOUND:
				settings.password[0] = '\0';
				ESP_LOGI(TAG, "The password is not initialized yet. Setting first byte to null");
				break;
			default:
				ESP_LOGE(TAG, "Error %s reading password", esp_err_to_name(err));
		}
	}

	nvs_close(load_handle);
}

void SaveSettings() {
	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		// NVS partition was truncated and needs to be erased
		// Retry nvs_flash_init
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
	}
	ESP_ERROR_CHECK(err);

	nvs_handle save_handle;
	err = nvs_open("storage", NVS_READWRITE, &save_handle);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Error %s opening NVS handle", esp_err_to_name(err));
	} else {
		err = nvs_set_u16(save_handle, "universeStart", settings.universeStart);
		if (err != ESP_OK) {
			ESP_LOGE(TAG, "Error %s saving universeStart", esp_err_to_name(err));
		}

		err = nvs_set_u16(save_handle, "universeStop", settings.universeStop);
		if (err != ESP_OK) {
			ESP_LOGE(TAG, "Error %s saving universeStop", esp_err_to_name(err));
		}

		err = nvs_set_u16(save_handle, "dmx_start", settings.dmx_start);
		if (err != ESP_OK) {
			ESP_LOGE(TAG, "Error %s saving dmx_start", esp_err_to_name(err));
		}

		err = nvs_set_u16(save_handle, "pixel_number", settings.pixel_number);
		if (err != ESP_OK) {
			ESP_LOGE(TAG, "Error %s saving pixel_number", esp_err_to_name(err));
		}

		err = nvs_set_u16(save_handle, "selected_pattern", settings.selected_pattern);
		if (err != ESP_OK) {
			ESP_LOGE(TAG, "Error %s saving selected_pattern", esp_err_to_name(err));
		}

		err = nvs_set_str(save_handle, "ssid", &settings.ssid);
		if (err != ESP_OK) {
			ESP_LOGE(TAG, "Error %s saving ssid", esp_err_to_name(err));
		}

		err = nvs_set_str(save_handle, "password", &settings.password);
		if (err != ESP_OK) {
			ESP_LOGE(TAG, "Error %s saving password", esp_err_to_name(err));
		}

		err = nvs_commit(save_handle);
		if (err != ESP_OK) {
			ESP_LOGE(TAG, "Error %s committing", esp_err_to_name(err));
		}
	}

	nvs_close(save_handle);
}

