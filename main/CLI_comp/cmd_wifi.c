#include "argtable3/argtable3.h"
#include "esp_console.h"
#include "esp_log.h"
#include "string.h"
#include "../settings.h"


static struct {
    struct arg_str * ssid;
    struct arg_str * pass;
	struct arg_end * end;
} wifi_args;

static void printSettings() {
	ESP_LOGI(__func__,"Stored Settings\n  ssid: %s, password %s", settings.ssid, settings.password);
}

static int set_wifi_command(int argc, char** argv) {
	int nerrors = arg_parse(argc, argv, (void**) &wifi_args);
	if (nerrors != 0) {
		arg_print_errors(stderr, wifi_args.end, argv[0]);
		printSettings();
		return 1;
	}
	
	if (*wifi_args.ssid->sval[0] != '\0') {
        strcpy(&settings.ssid, wifi_args.ssid->sval[0]);
        ESP_LOGI(__func__,"ssid set to: %s", settings.ssid);
    }

    if (*wifi_args.pass->sval[0] != '\0') {
        strcpy(&settings.password, wifi_args.pass->sval[0]);
        ESP_LOGI(__func__,"password set to: %s", settings.password);
    }

	SaveSettings();
    ESP_LOGI(__func__,"Settings stored!");
	return 0;
}

void register_wifi() {
	wifi_args.ssid = arg_str0("s", "ssid", "<ssid>", "WiFi SSID");
    wifi_args.pass = arg_str0("p", "pass", "<password>", "WiFi password");

    wifi_args.end = arg_end(2);
	
	const esp_console_cmd_t set_wifi_cmd = { 
		.command = "wifi",
		.help = "Set the SSID and/or password",
		.hint = NULL,
		.func = set_wifi_command,
		.argtable = &wifi_args
	};
	esp_console_cmd_register(&set_wifi_cmd);
}

