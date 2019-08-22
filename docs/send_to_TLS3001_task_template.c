#include "TLS3001.h"

//Create a massage structure
static pixel_message_s data_packet;

//Allocate a local color array buffer to be used as storing the color data.
static uint16_t local_color_array[PIXELS_CONNECTED*3];


void send_sample_color()
{
    uint16_t num_pixels = 10;   //Pixels to light up

    //Generate an orange color for all pixels
    uint16_t red = 3000;
    uint16_t green = 1500;
    uint16_t blue = 0;

    //Create a pointer to send over the queue to the TLS3001_task
    pixel_message_s *data_packet_tp;

    //Set pointer to point at local pattern data structure
    data_packet_tp = &data_packet;

    //Fill local_color_array with equal color (orange)
    for (size_t i = 0; i < num_pixels; i++)
	{
        local_color_array[(i*3)+0] = red;
        local_color_array[(i*3)+1] = green;
        local_color_array[(i*3)+2] = blue;
	}


    if( xSemaphoreTake(data_packet_tp->data_semaphore_guard, ( TickType_t ) 10 ) == pdTRUE )
    {
        ESP_LOGI(TAG, "Generating equal color data");

        data_packet_tp->color_data_p = &local_color_array;
        data_packet_tp->pixel_len = num_pixels;
        
        //Done with the data
        xSemaphoreGive(data_packet_tp->data_semaphore_guard);

        //Send copy of pointer to pixel_message_s structure to TLS3001 task
        if(xQueueSend(TLS3001_input_queue, (void *) &data_packet_tp,(TickType_t )10))
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