#include "cmd_TLS3001.h"
#include "argtable3/argtable3.h"
#include "esp_console.h"
#include "../TLS3001.h"
#include "esp_log.h"
#include "string.h"
#include "../pattern_generator.h"
#include "../settings.h"


static struct {
    struct arg_int * universeStart;
	struct arg_int * universeStop;
	struct arg_int * DMXStart;
	struct arg_end * end;
} e131_args;

static void printSettings() {
	ESP_LOGI(__func__,"Stored Settings\nUniverse start: %d\n Universe stop: %d\n DMX channel start: %d",
	settings.universeStart, settings.universeStop, settings.dmx_start);
}

static int set_e131_command(int argc, char** argv) {
	int nerrors = arg_parse(argc, argv, (void**) &e131_args);
	if (nerrors != 0) {
		arg_print_errors(stderr, e131_args.end, argv[0]);
		printSettings();
		return 1;
	}
	
	if (e131_args.universeStart->ival[0] < 1) {
        ESP_LOGE(__func__, "Start universe must be 1 or greater");
		return 1;
    }

	if (e131_args.DMXStart->ival[0] < 0) {
        ESP_LOGE(__func__, "DMX start channel must be > 0");
		return 1;
    }

	if ( (e131_args.universeStop->ival[0] - e131_args.universeStart->ival[0]) < 1) {
        ESP_LOGE(__func__, "The totat number of universes must be > 1");
		return 1;
    }

	settings.universeStart = e131_args.universeStart->ival[0];
	settings.universeStop = e131_args.universeStop->ival[0];
	settings.dmx_start = e131_args.DMXStart->ival[0];

	ESP_LOGI(__func__, "Start universe set to:%d\nStop universe set to:%d\nDMX channel start set to:%d\n", 
	settings.universeStart, settings.universeStop, settings.dmx_start);

	SaveSettings();
	ESP_LOGI(__func__,"Settings stored!");

	return 0;
}

void register_E131() {
	e131_args.universeStart = arg_int1("s", "universeStart", "<universeStart>", "E131 start universe");
	e131_args.universeStop = arg_int1("t", "universeStop", "<universeStop>", "E131 stop universe");
	e131_args.DMXStart = arg_int1("c", "DMXStart", "<DMXStart>", "E131 DMX Start channel");

    e131_args.end = arg_end(2);
	
	const esp_console_cmd_t set_mode_cmd = { 
		.command = "e131",
		.help = "Set the E1.31 start/stop universe and DMX channel start",
		.hint = NULL,
		.func = set_e131_command,
		.argtable = &e131_args
	};
	esp_console_cmd_register(&set_mode_cmd);
}

