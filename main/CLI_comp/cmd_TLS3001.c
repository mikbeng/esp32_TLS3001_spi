#include "cmd_TLS3001.h"
#include "argtable3/argtable3.h"
#include "esp_console.h"
#include "../TLS3001.h"
#include "esp_log.h"
#include "string.h"
#include "../pattern_generator.h"


static struct {
	struct arg_int * num_pixels;
	struct arg_end * end;
} pattern_pixel_number_args;

static struct {
	struct arg_int * pattern_default;
	struct arg_end * end;
} pattern_default_args;

static struct {
	struct arg_str * rgb;
	struct arg_end * end;
} pattern_equal_color_args;

static struct {
	struct arg_str * rgb;
	struct arg_int * delay;
	struct arg_end * end;
} pattern_running_lights_args;

static struct {
	struct arg_str * rgb;
	struct arg_int * delay;
	struct arg_end * end;
} pattern_colorWipe_args;

static struct {
	struct arg_int * width;
	struct arg_int * gap;
	struct arg_int * bpm;
	struct arg_int * flow;
	struct arg_int * hue;
	struct arg_int * dimmin;
	struct arg_int * dimmax;
	struct arg_end * end;
} pattern_artery_args;

static int pattern_pixel_number_command(int argc, char** argv) {
	int nerrors = arg_parse(argc, argv, (void**) &pattern_pixel_number_args);
	if (nerrors != 0) {
		arg_print_errors(stderr, pattern_pixel_number_args.end, argv[0]);
		return 1;
	}

	if (pattern_pixel_number_args.num_pixels->count > 0) {
		if (pattern_pixel_number_args.num_pixels->ival[0] > PIXELS_CONNECTED) {
			ESP_LOGE(__func__, "Number of pixels is higher than PIXELS_CONNECTED: %d", PIXELS_CONNECTED);
			return 1;
		}
		if (pattern_pixel_number_args.num_pixels->ival[0] < 3) {
			ESP_LOGE(__func__, "Number of pixels must be at least 2.");
			return 1;
		}
	} else {
		ESP_LOGE(__func__, "Number of pixels must be specified");
		return 1;
	}
	ESP_LOGI(__func__, "pixels: %d", pattern_pixel_number_args.num_pixels->ival[0]);
	pattern_set_pixel_number(pattern_pixel_number_args.num_pixels->ival[0]);

	return 0;
}

static int pattern_default_command(int argc, char** argv) {
	int nerrors = arg_parse(argc, argv, (void**) &pattern_default_args);
	if (nerrors != 0) {
		arg_print_errors(stderr, pattern_default_args.end, argv[0]);
		return 1;
	}

	if (pattern_default_args.pattern_default->count > 0) {
		if (pattern_default_args.pattern_default->ival[0] > 4) {
			ESP_LOGE(__func__, "Default pattern must be between 0 and x.");
			return 1;
		}
		if (pattern_default_args.pattern_default->ival[0] < 0) {
			ESP_LOGE(__func__, "Default pattern must be between 0 and x.");
			return 1;
		}
	} else {
		ESP_LOGE(__func__, "Default pattern must be specified.");
		return 1;
	}
	ESP_LOGI(__func__, "Default pattern set: %d", pattern_default_args.pattern_default->ival[0]);
	pattern_set_default(pattern_default_args.pattern_default->ival[0]);

	return 0;
}

static int pattern_RunningLights_command(int argc, char** argv) {
	const char s[2] = ","; 
	char *token_color;
	uint16_t int_color[3];
	int16_t delay_effect;

	int nerrors = arg_parse(argc, argv, (void**) &pattern_running_lights_args);
	if (nerrors != 0) {
		arg_print_errors(stderr, pattern_running_lights_args.end, argv[0]);
		return 1;
	}
	
	delay_effect = pattern_running_lights_args.delay->ival[0];

	token_color = strtok((char *)pattern_running_lights_args.rgb->sval[0], s);
	int_color[0] = atoi(token_color);

	token_color = strtok(NULL, s);
	int_color[1] = atoi(token_color);

	token_color = strtok(NULL, s);
	int_color[2] = atoi(token_color);
	
	ESP_LOGI(__func__,"%d,%d,%d",int_color[0],int_color[1],int_color[2]);

	pattern_set_effect(Running_Lights, int_color, delay_effect);

	//pattern_send_equal_color(int_color, set_all_pixel_color_args.num_pixels->ival[0]);
	//ESP_LOGI(__func__,"%d,%d,%d",set_all_pixel_color_args.red->ival[0], set_all_pixel_color_args.green->ival[0], set_all_pixel_color_args.blue->ival[0]);
	return 0;
}

static int pattern_colorWipe_command(int argc, char** argv) {
	const char s[2] = ","; 
	char *token_color;
	uint16_t int_color[3];
	int16_t delay_effect;

	int nerrors = arg_parse(argc, argv, (void**) &pattern_colorWipe_args);
	if (nerrors != 0) {
		arg_print_errors(stderr, pattern_colorWipe_args.end, argv[0]);
		return 1;
	}
	
	delay_effect = pattern_colorWipe_args.delay->ival[0];

	token_color = strtok((char *)pattern_colorWipe_args.rgb->sval[0], s);
	int_color[0] = atoi(token_color);

	token_color = strtok(NULL, s);
	int_color[1] = atoi(token_color);

	token_color = strtok(NULL, s);
	int_color[2] = atoi(token_color);
	
	ESP_LOGI(__func__,"%d,%d,%d",int_color[0],int_color[1],int_color[2]);
   
	pattern_set_effect(colorWipe, int_color, delay_effect);
	
	//pattern_send_equal_color(int_color, set_all_pixel_color_args.num_pixels->ival[0]);
	//ESP_LOGI(__func__,"%d,%d,%d",set_all_pixel_color_args.red->ival[0], set_all_pixel_color_args.green->ival[0], set_all_pixel_color_args.blue->ival[0]);
	return 0;
}

static int pattern_equal_color_command(int argc, char** argv) {
	const char s[2] = ","; 
	char *token_color;
	uint16_t int_color[3];

	int nerrors = arg_parse(argc, argv, (void**) &pattern_equal_color_args);
	if (nerrors != 0) {
		arg_print_errors(stderr, pattern_equal_color_args.end, argv[0]);
		return 1;
	}

	token_color = strtok((char *)pattern_equal_color_args.rgb->sval[0], s);
	int_color[0] = atoi(token_color);

	token_color = strtok(NULL, s);
	int_color[1] = atoi(token_color);

	token_color = strtok(NULL, s);
	int_color[2] = atoi(token_color);

	ESP_LOGI(__func__,"%d,%d,%d",int_color[0],int_color[1],int_color[2]);

	pattern_set_effect(equal_color, int_color, 0);

	//pattern_send_equal_color(int_color, set_all_pixel_color_args.num_pixels->ival[0]);
	//ESP_LOGI(__func__,"%d,%d,%d",set_all_pixel_color_args.red->ival[0], set_all_pixel_color_args.green->ival[0], set_all_pixel_color_args.blue->ival[0]);
	return 0;
}

static int pattern_artery_command(int argc, char** argv) {
	int nerrors = arg_parse(argc, argv, (void**) &pattern_artery_args);
	if (nerrors != 0) {
		arg_print_errors(stderr, pattern_artery_args.end, argv[0]);
		return 1;
	}

	int16_t width = -1;
	int16_t gap = -1;
	int16_t bpm = -1;
	int16_t flow = -1;
	int16_t hue = -1;
	int16_t dimmin = -1;
	int16_t dimmax = -1;
	if (pattern_artery_args.width->count == 1) {
		if (pattern_artery_args.width->ival[0] > 0 &&
			pattern_artery_args.width->ival[0] <= 255) {
			width = pattern_artery_args.width->ival[0];
		} else {
			ESP_LOGE(__func__, "Width must be between 1-255");
			return 1;
		}
	}
	if (pattern_artery_args.gap->count == 1) {
		if (pattern_artery_args.gap->ival[0] > 0 &&
			pattern_artery_args.gap->ival[0] <= 255) {
			gap = pattern_artery_args.gap->ival[0];
		} else {
			ESP_LOGE(__func__, "Gap must be between 1-255");
			return 1;
		}
	}
	if (pattern_artery_args.bpm->count == 1) {
		if (pattern_artery_args.bpm->ival[0] > 0 &&
			pattern_artery_args.bpm->ival[0] <= 255) {
			bpm = pattern_artery_args.bpm->ival[0];
		} else {
			ESP_LOGE(__func__, "BPM must be between 1-255");
			return 1;
		}
	}
	if (pattern_artery_args.flow->count == 1) {
		if (pattern_artery_args.flow->ival[0] >= 0 &&
			pattern_artery_args.flow->ival[0] <= 255) {
			flow = pattern_artery_args.flow->ival[0];
		} else {
			ESP_LOGE(__func__, "Flow must be between 0-255");
			return 1;
		}
	}
	if (pattern_artery_args.hue->count == 1) {
		if (pattern_artery_args.hue->ival[0] >= 0 &&
			pattern_artery_args.hue->ival[0] <= 255) {
			hue = pattern_artery_args.hue->ival[0];
		} else {
			ESP_LOGE(__func__, "HUE must be between 0-255");
			return 1;
		}
	}
	if (pattern_artery_args.dimmin->count == 1) {
		if (pattern_artery_args.dimmin->ival[0] >= 0 &&
			pattern_artery_args.dimmin->ival[0] <= 255) {
			dimmin = pattern_artery_args.dimmin->ival[0];
		} else {
			ESP_LOGE(__func__, "Dim min must be between 0-255");
			return 1;
		}
	}
	if (pattern_artery_args.dimmax->count == 1) {
		if (pattern_artery_args.dimmax->ival[0] >= 0 &&
			pattern_artery_args.dimmax->ival[0] <= 255) {
			dimmax = pattern_artery_args.dimmax->ival[0];
		} else {
			ESP_LOGE(__func__, "Dim max must be between 0-255");
			return 1;
		}
	}
	ESP_LOGI(__func__, "artery width: %d gap: %d bpm: %d flow: %d hue: %d dimmin: %d dimmax: %d",
			width, gap, bpm, flow, hue, dimmin, dimmax);
	pattern_artery(width, gap, bpm, flow, hue, dimmin, dimmax);

	return 0;
}

void register_TLS3001() {
	// ------------  set pixel number command
	pattern_pixel_number_args.num_pixels = arg_int1("p", "num_pixels", "<num_pixels>", "number of pixels to light. Will be stored in settings.");
	pattern_pixel_number_args.end = arg_end(2);
	
	const esp_console_cmd_t set_pixel_number_cmd = { 
		.command = "pattern_pixel_number",
		.help = "Set total pixel number to be active",
		.hint = NULL,
		.func = pattern_pixel_number_command,
		.argtable = &pattern_pixel_number_args
	};
	esp_console_cmd_register(&set_pixel_number_cmd);

	// ------------  set default_pattern command
	pattern_default_args.pattern_default = arg_int1("p", "pattern_default", "<pattern_default>", "Set default pattern. Will be stored in settings.");
	pattern_default_args.end = arg_end(2);
	
	const esp_console_cmd_t set_pattern_default_cmd = { 
		.command = "pattern_default",
		.help = "Set default pattern.",
		.hint = NULL,
		.func = pattern_default_command,
		.argtable = &pattern_default_args
	};
	esp_console_cmd_register(&set_pattern_default_cmd);

	// ------------  set equal color command
	pattern_equal_color_args.rgb = arg_str1(NULL, NULL, "<r,g,b>", "rgb color data values 12 bits (0-4095)");
	pattern_equal_color_args.end = arg_end(2);
	
	const esp_console_cmd_t equal_color_cmd = { 
		.command = "pattern_equal_color",
		.help = "Set same color to pixels",
		.hint = NULL,
		.func = pattern_equal_color_command,
		.argtable = &pattern_equal_color_args
	};
	esp_console_cmd_register(&equal_color_cmd);

	// ------------  running light command
	pattern_running_lights_args.rgb = arg_str1(NULL, NULL, "<r,g,b>", "rgb color data values 12 bits (0-4095)");
	pattern_running_lights_args.delay = arg_int1("d", "delay", "<delay>", "set delay effect");
	pattern_running_lights_args.end = arg_end(2);

	const esp_console_cmd_t running_lights_cmd = { 
		.command = "pattern_RunningLights",
		.help = "Set running lights effect",
		.hint = NULL,
		.func = pattern_RunningLights_command,
		.argtable = &pattern_running_lights_args
	};
	esp_console_cmd_register(&running_lights_cmd);

	// ------------  Color Wipe command
	pattern_colorWipe_args.rgb = arg_str1(NULL, NULL, "<r,g,b>", "rgb color data values 12 bits (0-4095)");
	pattern_colorWipe_args.delay = arg_int1("d", "delay", "<delay>", "set delay effect");
	pattern_colorWipe_args.end = arg_end(2);
	
	const esp_console_cmd_t colorWipe_cmd = { 
		.command = "pattern_ColorWipe",
		.help = "Set color wipe effect",
		.hint = NULL,
		.func = pattern_colorWipe_command,
		.argtable = &pattern_colorWipe_args
	};
	esp_console_cmd_register(&colorWipe_cmd);

	// ------------  set default_pattern command
	pattern_artery_args.width = arg_int0("w", "width", "<1-255>", "Width of beat");
	pattern_artery_args.gap = arg_int0("g", "gap", "<1-255>", "Gap between beats");
	pattern_artery_args.bpm = arg_int0("b", "bpm", "<1-255>", "Beats flash in BPM");
	pattern_artery_args.flow = arg_int0("f", "flow", "<0-255>", "Flow of beats");
	pattern_artery_args.hue = arg_int0("h", "hue", "<0-255>", "Color of beats");
	pattern_artery_args.dimmin = arg_int0("n", "dimmin", "<0-255>", "Minimum dim level of beat");
	pattern_artery_args.dimmax = arg_int0("m", "dimmax", "<0-255>", "Maximum dim level of beat");
	pattern_artery_args.end = arg_end(2);

	const esp_console_cmd_t pattern_artery_cmd = {
		.command = "pattern_artery",
		.help = "Start artery pattern with flashing beats.",
		.hint = NULL,
		.func = pattern_artery_command,
		.argtable = &pattern_artery_args
	};
	esp_console_cmd_register(&pattern_artery_cmd);
}

