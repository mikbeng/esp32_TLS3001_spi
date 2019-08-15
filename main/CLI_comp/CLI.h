#pragma once

#define CONFIG_STORE_HISTORY 0
#define CONFIG_CONSOLE_UART_BAUDRATE 115200
#define CONFIG_CLI_CORE 0

void start_cli();
void stop_cli(void);
void start_cli_passive_mode(void);