#include "main.h"
#include "string.h"
#include "stdint.h"
#include "stdbool.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "TLS3001.h"
#include "pattern_generator.h"

static const char * TAG = "pattern_gen";

static void pattern_gen_task(void *arg);

//Create a massage structure for this module (pattern_generator)
static pixel_message_s pattern_data_packet;

//Allocate a local color array buffer to be used as storing the color data.
static uint16_t pattern_color_array[PIXELS_CONNECTED*3];

// Pattern_gen_task. Not used at the moment.
void pattern_gen_task(void *arg)
{
   while(1)
    {
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }        
}

esp_err_t pattern_init(uint16_t num_pixels)
{
    pattern_data_packet.data_semaphore_guard = xSemaphoreCreateMutex();
	
    //xTaskCreate(&pattern_gen_task, "pattern_gen_task", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "pattern_gen initiated!");

    return ESP_OK;
}

void pattern_send_equal_color(uint16_t *rgb, uint16_t num_pixels)
{
    uint16_t red = *rgb;
    uint16_t green = *(rgb+1);
    uint16_t blue = *(rgb+2);

    //Create a pointer to send over the queue
    pixel_message_s *pattern_data_packet_tp;

    //Set pointer to point at local pattern data structure
    pattern_data_packet_tp = &pattern_data_packet;

    for (size_t i = 0; i < num_pixels; i++)
	{
        pattern_color_array[(i*3)+0] = red;
        pattern_color_array[(i*3)+1] = green;
        pattern_color_array[(i*3)+2] = blue;
	}


    if( xSemaphoreTake(pattern_data_packet_tp->data_semaphore_guard, ( TickType_t ) 10 ) == pdTRUE )
    {
        ESP_LOGI(TAG, "Generating equal color data");

        pattern_data_packet_tp->color_data_p = &pattern_color_array;
        pattern_data_packet_tp->pixel_len = num_pixels;
        
        //Done with the data
        xSemaphoreGive(pattern_data_packet_tp->data_semaphore_guard);

        //Send copy of pointer to pixel_message_s structure to TLS3001 task
        if(xQueueSend(TLS3001_input_queue, (void *) &pattern_data_packet_tp,(TickType_t )10))
        {
            ESP_LOGI(TAG, "successfully posted pattern data on queue");
        }
        else
        {
            ESP_LOGW(TAG, "Queue full. Did not post any data!");
        }        

    }
    else
    {
        //The semaphore could not be taken. This would mean that the TLS3001_task is still processing the data.
        ESP_LOGW(TAG, "Semaphore busy! TLS3001_task still processing data");
    }
    

}


void pattern_shift_pixels_right(void *spi_tx_data_start, uint16_t pixel_index_start, uint16_t pixel_index_end, uint16_t num_pixels)
{
    //Not implemented yet.    
} 