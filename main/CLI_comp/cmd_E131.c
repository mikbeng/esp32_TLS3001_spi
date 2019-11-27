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
	struct arg_end * end;
} e131_args;

static void printSettings() {
	ESP_LOGI(__func__,"Stored Settings\n  Universe start: %d", settings.universeStart);
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
	ESP_LOGI(__func__, "Arg universe %d", e131_args.universeStart->ival[0]);

	settings.universeStart = e131_args.universeStart->ival[0];
	SaveSettings();
	return 0;
}

void register_E131() {
	e131_args.universeStart = arg_int1("s", "universeStart", "<universeStart>", "E131 start universe");

    e131_args.end = arg_end(2);
	
	const esp_console_cmd_t set_mode_cmd = { 
		.command = "e131",
		.help = "Set the start E1.31 universe to trigger on",
		.hint = NULL,
		.func = set_e131_command,
		.argtable = &e131_args
	};
	esp_console_cmd_register(&set_mode_cmd);
}

