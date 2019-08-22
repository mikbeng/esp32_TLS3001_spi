#include "cmd_TLS3001.h"
#include "argtable3/argtable3.h"
#include "esp_console.h"
#include "../TLS3001.h"
#include "esp_log.h"
#include "string.h"
#include "../pattern_generator.h"


static struct {
    struct arg_int * universeStart;
	struct arg_end * end;
} e131_args;


static int set_e131_command(int argc, char** argv) {
	int nerrors = arg_parse(argc, argv, (void**) &e131_args);
	if (nerrors != 0) {
		arg_print_errors(stderr, e131_args.end, argv[0]);
		return 1;
	}
	
	if (e131_args.universeStart->count < 1) {
        ESP_LOGE(__func__, "Start universe must be 1 or greater");
		return 1;
    }
	ESP_LOGI(__func__, "Arg universe %d", e131_args.universeStart->ival[0]);

    //ESP_LOGI(__func__,"%d,%d,%d",int_color[0],int_color[1],int_color[2]);
    //pattern_send_equal_color(int_color, e131_args.num_pixels->ival[0]);
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

