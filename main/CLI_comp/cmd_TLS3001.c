#include "cmd_TLS3001.h"
#include "argtable3/argtable3.h"
#include "esp_console.h"
#include "../TLS3001.h"
#include "esp_log.h"
#include "string.h"
#include "../pattern_generator.h"


static struct {
	struct arg_str * rgb;
    struct arg_int * num_pixels;
	struct arg_end * end;
} set_all_pixel_color_args;


static int pixels_set_color_command(int argc, char** argv) {
    const char s[2] = ","; 
    char *token_color;
    uint16_t int_color[3];

	int nerrors = arg_parse(argc, argv, (void**) &set_all_pixel_color_args);
	if (nerrors != 0) {
		arg_print_errors(stderr, set_all_pixel_color_args.end, argv[0]);
		return 1;
	}
	
	if (set_all_pixel_color_args.num_pixels->count > 0) {
        if(set_all_pixel_color_args.num_pixels->ival[0] > PIXELS_CONNECTED)
        {
            ESP_LOGE(__func__, "Number of pixels is higher than PIXELS_CONNECTED: %d", PIXELS_CONNECTED);
            return 1;
        }
	}
    else
    {
        ESP_LOGE(__func__, "Number of pixels must be specified");
		return 1;
    }

    token_color = strtok((char *)set_all_pixel_color_args.rgb->sval[0], s);
    int_color[0] = atoi(token_color);

    token_color = strtok(NULL, s);
    int_color[1] = atoi(token_color);

    token_color = strtok(NULL, s);
    int_color[2] = atoi(token_color);
    
    ESP_LOGI(__func__,"%d,%d,%d",int_color[0],int_color[1],int_color[2]);
   
    pattern_send_equal_color(int_color, set_all_pixel_color_args.num_pixels->ival[0]);
	//ESP_LOGI(__func__,"%d,%d,%d",set_all_pixel_color_args.red->ival[0], set_all_pixel_color_args.green->ival[0], set_all_pixel_color_args.blue->ival[0]);
	return 0;
}

void register_TLS3001() {

    // ------------  set pixel color command
	set_all_pixel_color_args.rgb = arg_str1(NULL, NULL, "<r,g,b>", "rgb color data values 12 bits (0-4095)");
    //set_all_pixel_color_args.green = arg_int1("g", "green", "<green>", "green color data value 12 bits (0-4095)");
    //set_all_pixel_color_args.blue = arg_int1("b", "blue", "<blue>", "blue color data value 12 bits (0-4095)");
	set_all_pixel_color_args.num_pixels = arg_int1("p", "num_pixels", "<num_pixels>", "number of pixels to lit");

    set_all_pixel_color_args.end = arg_end(2);
	
	const esp_console_cmd_t set_mode_cmd = { 
		.command = "pixels_set_color",
		.help = "Set same color to p number of pixels",
		.hint = NULL,
		.func = pixels_set_color_command,
		.argtable = &set_all_pixel_color_args
	};
	esp_console_cmd_register(&set_mode_cmd);
}

