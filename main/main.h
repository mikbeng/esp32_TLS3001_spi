#pragma once

#include "TLS3001.h"
#include "freertos/queue.h"

/*
typedef struct
{
	TLS3001_handle_s *TLS3001_ch1;
	TLS3001_handle_s *TLS3001_ch2;
	QueueHandle_t   TLS3001_data_in_queue;	
    TaskHandle_t    TLS3001_task_handle;

}TLS3001_Ctrl_s;
*/

#define TEST_PIXELS 2

extern QueueHandle_t  TLS3001_input_queue;

#define BLINK_GPIO CONFIG_BLINK_GPIO
