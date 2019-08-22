#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "CLI.h"
#include "CLI_cmd_decs.h"

static const char *TAG = "CLI";
TaskHandle_t xCLI_handle = NULL;

// Passive mode
uint8_t rxbuf[256];
uint8_t num_detections = 0;
uint8_t activation_phrase_length = 3;
static const char *activation_phrase = "cli";
TaskHandle_t x_cli_passive_handle = NULL;
QueueHandle_t uart_queue = NULL;

// CLI task
bool cli_task_stop_flag = false;

#if CONFIG_STORE_HISTORY
#include "esp_vfs_fat.h"
#define MOUNT_PATH "/data"
#define HISTORY_PATH MOUNT_PATH "/history.txt"

static void initialize_filesystem() {
	static wl_handle_t wl_handle;
	const esp_vfs_fat_mount_config_t mount_config = {
		.max_files = 4,
		.format_if_mount_failed = true,
	};
	
	esp_err_t ret = esp_vfs_fat_spiflash_mount(MOUNT_PATH, "storage", &mount_config, &wl_handle);
	
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(ret));
	}
}

#endif //CONFIG_STORE_HISTORY

void register_custom_commands() {
	/*	Register custom commands here */
	register_system();
	register_TLS3001();
	register_E131();
}

static void initialize_nvs() {
	esp_err_t ret = nvs_flash_init();
	
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Failed to initialize NVS Flash (%s)", esp_err_to_name(ret));
	}
}

static void initialize_cli() {
	// Disable bufering on stdin/out
	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0);
	
	/* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
	esp_vfs_dev_uart_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
	/* Move the caret to the beginning of the next line on '\n' */
	esp_vfs_dev_uart_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);
	
	
	/* Configure UART. Note that REF_TICK is used so that the baud rate remains
	 * correct while APB frequency is changing in light sleep mode.
	 */
	const uart_config_t uart_config = {
		.baud_rate = CONFIG_CONSOLE_UART_BAUDRATE,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.use_ref_tick = true
	};
	ESP_ERROR_CHECK(uart_param_config(CONFIG_CONSOLE_UART_NUM, &uart_config));

	/* Install UART driver for interrupt-driven reads and writes */
	ESP_ERROR_CHECK(uart_driver_install(CONFIG_CONSOLE_UART_NUM,
		256,
		0,
		0,
		NULL,
		0));

	/* Tell VFS to use UART driver */
	esp_vfs_dev_uart_use_driver(CONFIG_CONSOLE_UART_NUM);
	
	
	// Initialize the console
	esp_console_config_t console_config = {
		.max_cmdline_args = 8,
		.max_cmdline_length = 256,
#if CONFIG_LOG_COLORS
		.hint_color = atoi(LOG_COLOR_CYAN),
#endif // CONFIG_LOG_COLORS
	};
	
	ESP_ERROR_CHECK(esp_console_init(&console_config));
	
	/* Configure linenoise line completion library */
	/* Enable multiline editing. If not set, long commands will scroll within
	 * single line.
	 */
	linenoiseSetMultiLine(1);

	/* Tell linenoise where to get command completions and hints */
	linenoiseSetCompletionCallback(&esp_console_get_completion);
	linenoiseSetHintsCallback((linenoiseHintsCallback*) &esp_console_get_hint);

	/* Set command history size */
	linenoiseHistorySetMaxLen(100);

#if CONFIG_STORE_HISTORY
	/* Load command history from filesystem */
	linenoiseHistoryLoad(HISTORY_PATH);
#endif
}

void cli_init() {
	initialize_nvs();
	
#if CONFIG_STORE_HISTORY
	initialize_filesystem();
#endif
	
	cli_task_stop_flag = false;
	initialize_cli();
	
	esp_console_register_help_command();
	
	
	// Register custom commands from other components
	register_custom_commands();
}

void cli_task(void *p) {
	/* Prompt to be printed before each line.
	 * This can be customized, made dynamic, etc.
	 */
	const char* prompt = LOG_COLOR_I "TLS3001_driver> " LOG_RESET_COLOR;
	
	// Welcome message
	printf("\n"
"  @@@@@@@  @@@        @@@@@@         @@@@@@    @@@@@@@@    @@@@@@@@     @@@\n"
"  @@@@@@@  @@@       @@@@@@@         @@@@@@@  @@@@@@@@@@  @@@@@@@@@@   @@@@\n"
"    @@!    @@!       !@@                 @@@  @@!   @@@@  @@!   @@@@  @@@!!\n"
"    !@!    !@!       !@!                 @!@  !@!  @!@!@  !@!  @!@!@    !@!\n"
"    @!!    @!!       !!@@!!          @!@!!@   @!@ @! !@!  @!@ @! !@!    @!@\n"
"    !!!    !!!        !!@!!!         !!@!@!   !@!!!  !!!  !@!!!  !!!    !@!\n"
"    !!:    !!:            !:!            !!:  !!:!   !!!  !!:!   !!!    !!:\n"
"    :!:     :!:          !:!             :!:  :!:    !:!  :!:    !:!    :!:\n"
"     ::     :: ::::  :::: ::         :: ::::  ::::::: ::  ::::::: ::    :::\n"
"     :     : :: : :  :: : :           : : :    : : :  :    : : :  :      ::\n"
	);
	
	printf("\n"
	       "Welcome to Command Line Interface.\n"
	       "Type 'help' to get the list of commands.\n"
	       "Use UP/DOWN arrows to navigate through command history.\n"
	       "Press TAB when typing command name to auto-complete.\n");
	
	
	/* Figure out if the terminal supports escape sequences */
	int probe_status = linenoiseProbe();
	if (probe_status) {
		/* zero indicates success */
		printf("\n"
		       "Your terminal application does not support escape sequences.\n"
		       "Line editing and history features are disabled.\n"
		       "On Windows, try using Putty instead.\n");
		linenoiseSetDumbMode(1);
#if CONFIG_LOG_COLORS
		/* Since the terminal doesn't support escape sequences,
		 * don't use color codes in the prompt.
		 */
		prompt = "pmstep> ";
#endif //CONFIG_LOG_COLORS
	}
	
	/* Main loop */
	while (!cli_task_stop_flag) {
		/* Get a line using linenoise.
		 * The line is returned when ENTER is pressed.
		 */
		char *line = linenoise(prompt);
		if (line == NULL) {
			/* Ignore empty lines */
			continue;
		}
		/* Add the command to the history */
		linenoiseHistoryAdd(line);
#if CONFIG_STORE_HISTORY
		/* Save command history to filesystem */
		linenoiseHistorySave(HISTORY_PATH);
#endif

		/* Try to run the command */
		int ret;
		esp_err_t err = esp_console_run(line, &ret);
		if (err == ESP_ERR_NOT_FOUND) {
			printf("Unrecognized command\n");
		}
		else if (err == ESP_ERR_INVALID_ARG) {
			// command was empty
		}
		else if (err == ESP_OK && ret != ESP_OK) {
			printf("Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(err));
		}
		else if (err != ESP_OK) {
			printf("Internal error: %s\n", esp_err_to_name(err));
		}
		/* linenoise allocates line buffer on the heap, so need to free it */
		linenoiseFree(line);
		
	}
	ESP_LOGI(TAG, "cli_task_stop_flag = %d", cli_task_stop_flag);

	ESP_LOGI(TAG, "CLI stop request received. Exiting CLI...");
	
	ESP_LOGI(TAG, "\t>> Stopping console...");
	esp_console_deinit();
	
	ESP_LOGI(TAG, "\t>> Deleting task. \n\n Good bye\n");
	vTaskDelete(xCLI_handle);
}

void start_cli() {
	cli_init();
	ESP_LOGI(TAG, "CLI started");
	xTaskCreatePinnedToCore(&cli_task, "CLI_Task", 8192, NULL, 15, &xCLI_handle, CONFIG_CLI_CORE);
}

void stop_cli() {
	if (!cli_task_stop_flag) {
		ESP_LOGI(TAG, "Rquesting CLI close...");
		cli_task_stop_flag = true;
	}
	else {
		ESP_LOGW(TAG, "CLI is not running");
	}
}

/*
 * Code for cli passive mode.
 *		Passive mode waits for a activation phrase
 *		before initializing.
 **/
bool check_uart_data(uint8_t * buf, size_t len) {
	for (int i = 0; i < len; i++) {
		if ((buf[i] >= 'a' && buf[i] <= 'z') || (buf[i] >= 'A' && buf[i] <= 'Z')) {
			// Valid char
						
			// Check if activation phrase
		   if(buf[i] == activation_phrase[num_detections]) {
				num_detections++;
			}
			else {
				num_detections = 0;
			}
			
			if(num_detections == activation_phrase_length) {
				return true;
			}
		}	
	}
	return false;
}

void cli_passive_task(void* p) {
	uart_event_t event;
	uint8_t* rxbuf = (uint8_t*) malloc(256);	//Increase this buffer to PIXELS*6 (2 bytes for each color, 3 colors in each pixel). Also maybe define it as a global so that it's not allocated on the stack.
	while (1) {
		if (xQueueReceive(uart_queue, &event, portMAX_DELAY)) {
			bzero(rxbuf, 256);
			switch (event.type) {
				case UART_DATA:
					uart_read_bytes(CONFIG_CONSOLE_UART_NUM, rxbuf, event.size, portMAX_DELAY);
					
					// Check if rxbuf contains activation phrase for CLI
					if (check_uart_data(rxbuf, event.size)) {
						
						ESP_LOGI(TAG, "match");
						
						// Delete driver so CLI can install it's own
						uart_driver_delete(CONFIG_CONSOLE_UART_NUM);
						
						// Start CLI
						start_cli();
						
						// Kill this task
						vTaskDelete(x_cli_passive_handle);
					}

					//uint16_t TLS3001_color_array[PIXELS_CONNECTED*3];
					//
					//TLS3001_uart_binary_parse(rxbuf, event.size, &TLS3001_color_array)	//Parses incomming binary uart data and fills the TLS3001_color_array 
					//Then send it to the TLS3001_input_queue as in pattern_send_equal_color()
					//finish by flushing the uart?
					break;
				default:
					break;
			}
		
		}
	}
	
}

void start_cli_passive_mode() {
	num_detections = 0;
	
	ESP_LOGI(TAG, "Starting CLI P4551V3 listener");
	
	vTaskDelay(10 / portTICK_PERIOD_MS); // Puase to prevent previous messega being corrupted from uart config.
	
	const uart_config_t uart_config = {
		.baud_rate = CONFIG_CONSOLE_UART_BAUDRATE,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.use_ref_tick = true
	};
	ESP_ERROR_CHECK(uart_param_config(CONFIG_CONSOLE_UART_NUM, &uart_config));

	
	uart_driver_delete(CONFIG_CONSOLE_UART_NUM);
	/* Install UART driver for interrupt-driven reads and writes */
	ESP_ERROR_CHECK(uart_driver_install(CONFIG_CONSOLE_UART_NUM,
		256,
		0,
		1,
		&uart_queue,
		0));
	
	xTaskCreatePinnedToCore(cli_passive_task, "cli_passive_task", 4096, NULL, 10, &x_cli_passive_handle, CONFIG_CLI_CORE);	
	ESP_LOGI(TAG, "Setup complete. Activation Phrase = %s",activation_phrase);
}

