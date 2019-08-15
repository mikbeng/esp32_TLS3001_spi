#include "cmd_system.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "inttypes.h"
#include "string.h"

#include "esp_console.h"

#include "CLI.h"




static int system_tasks_command(int argc, char** argv) {
	
	TaskStatus_t * status_array;
	uint32_t ulTotalRunTime;
	size_t num_tasks = uxTaskGetNumberOfTasks();
	status_array = malloc(num_tasks * sizeof(TaskStatus_t));
	
	num_tasks = uxTaskGetSystemState( status_array,
		num_tasks,
		&ulTotalRunTime);
	//ulTotalRunTime /= 100UL;
	
	if (ulTotalRunTime == 0)
		return 0;
	
	printf("TaskID | State | CurrentPrio | BasePrio | runtime |\tTaskName\n");
	
	/* For each populated position in the pxTaskStatusArray array,
		format the raw data as human readable ASCII data. */
	for (int x = 0; x < num_tasks; x++)
	{
		char task_state[20];
		switch (status_array[x].eCurrentState) {
		case eReady:
			strcpy(task_state, "\x1B[32mRDY\x1B[0m");
			break;
		case eRunning:
			strcpy(task_state, "\x1B[32mRUN\x1B[0m");
			break;
		case eBlocked:
			strcpy(task_state, "\x1B[33mBLK\x1B[0m");
			break;
		case eSuspended:
			strcpy(task_state, "\x1B[31mSUS\x1B[0m");
			break;
		case eDeleted:
			strcpy(task_state, "\x1B[36mDEL\x1B[0m");
			break;
		}
		
		float percent = 100.0f * ((float)status_array[x].ulRunTimeCounter) / ((float)ulTotalRunTime);
		
		printf("  %d\t  %s\t     %d\t          %d\t   %.1f\t\t%s\n", 
			status_array[x].xTaskNumber,
			task_state,
			status_array[x].uxCurrentPriority,
			status_array[x].uxBasePriority,
			percent,
			status_array[x].pcTaskName);
	}
	
	
	return 0;
}


static int system_reboot_comand(int argc, char** argv) {
	
	esp_restart();
	
	return 0;
}

static int system_exit_comand(int argc, char** argv) {
	
	//int clear_flag = 0;
	//flash_config_set_cli_flag(&clear_flag);
	esp_restart();
	
	return 0;
}


void register_system() {
	
	const esp_console_cmd_t reboot_cmd = {
			.command = "reboot",
		.help = "Reboots PMStep",
		.argtable = NULL,
		.func = system_reboot_comand,
		.hint = NULL
	};
	
	const esp_console_cmd_t exit_cmd = {
		.command = "exit",
		.help = "Exits cli",
		.argtable = NULL,
		.func = system_exit_comand,
		.hint = NULL
	};
	
	const esp_console_cmd_t task_cmd = {
		.command = "tasks",
		.help = "Prints all tasks",
		.argtable = NULL,
		.func = system_tasks_command,
		.hint = NULL
	};
	
	
	ESP_ERROR_CHECK(esp_console_cmd_register(&reboot_cmd));
	ESP_ERROR_CHECK(esp_console_cmd_register(&exit_cmd));
	ESP_ERROR_CHECK(esp_console_cmd_register(&task_cmd));
}