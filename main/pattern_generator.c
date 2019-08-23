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
#include "math.h"

#define NUM_LEDS PIXELS_CONNECTED

void RunningLights(uint16_t num_pixels, uint8_t red, uint8_t green, uint8_t blue, int WaveDelay);
void colorWipe(uint16_t num_pixels, uint8_t red, uint8_t green, uint8_t blue, int SpeedDelay);

static const char * TAG = "pattern_gen";

static void pattern_gen_task(void *arg);

//Create a massage structure for this module (pattern_generator)
static pixel_message_s pattern_data_packet;

//Allocate a local color array buffer to be used as storing the color data.
static uint16_t pattern_color_array[PIXELS_CONNECTED*3];

// Pattern_gen_task. Not used at the moment.
void pattern_gen_task(void *arg)
{   
   vTaskDelay(2000 / portTICK_PERIOD_MS); 
   //colorWipe(20,0x00,0xff,0x00, 50);
   while(1)
    {
        //RunningLights(10,0xff,0x00,0x00, 50);
        colorWipe(20,0xff,0xff,0x00, 1);
        colorWipe(20,0x00,0x00,0x00, 1);
        //vTaskDelay(2000 / portTICK_PERIOD_MS);
    }        
}

esp_err_t pattern_init(uint16_t num_pixels)
{
    pattern_data_packet.data_semaphore_guard = xSemaphoreCreateMutex();
	
    xTaskCreate(&pattern_gen_task, "pattern_gen_task", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "pattern_gen initiated!");

    return ESP_OK;
}

void delay(int delay){
    vTaskDelay(((uint32_t)delay) / portTICK_PERIOD_MS);
}

void setPixel(int Pixel, uint8_t red, uint8_t green, uint8_t blue) {

    //Convert 8bit to 12bit
    float ratio = 4095.0f/255.0f;
    pattern_color_array[(Pixel*3)+0] = (uint16_t) red*ratio;
    pattern_color_array[(Pixel*3)+1] = (uint16_t) green*ratio;;
    pattern_color_array[(Pixel*3)+2] = (uint16_t) blue*ratio;;
}

void showStrip(uint16_t num_pixels)
{
    /*
    ESP_LOGI(TAG, "showStrip:\n");
    for (size_t i = 0; i < num_pixels; i++)
    {
        ESP_LOGI(TAG, "pixel %d:[%d,%d,%d]",i,pattern_color_array[(i*3)+0],pattern_color_array[(i*3)+1],pattern_color_array[(i*3)+2]);
    }
    */
    TLS3001_show(&pattern_color_array, num_pixels);
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


void RunningLights(uint16_t num_pixels, uint8_t red, uint8_t green, uint8_t blue, int WaveDelay) {
  int Position=0;
  
  for(int i=0; i<num_pixels*2; i++)
  {
      Position++; // = 0; //Position + Rate;
      for(int i=0; i<num_pixels; i++) {
        // sine wave, 3 offset waves make a rainbow!
        //float level = sin(i+Position) * 127 + 128;
        //setPixel(i,level,0,0);
        //float level = sin(i+Position) * 127 + 128;
        setPixel(i,((sin(i+Position) * 127 + 128)/255)*red,
                   ((sin(i+Position) * 127 + 128)/255)*green,
                   ((sin(i+Position) * 127 + 128)/255)*blue);
      }
      
      showStrip(num_pixels);
      delay(WaveDelay);
  }
}

void colorWipe(uint16_t num_pixels, uint8_t red, uint8_t green, uint8_t blue, int SpeedDelay) {
  for(uint16_t i=0; i<num_pixels; i++) {
      setPixel(i, red, green, blue);
      showStrip(num_pixels);
      delay(SpeedDelay);
  }
}

/* 
void Fire(int Cooling, int Sparking, int SpeedDelay, uint16_t num_pixels) {
  static byte heat[num_pixels];
  int cooldown;
  
  // Step 1.  Cool down every cell a little
  for( int i = 0; i < num_pixels; i++) {
    cooldown = random(0, ((Cooling * 10) / num_pixels) + 2);
    
    if(cooldown>heat[i]) {
      heat[i]=0;
    } else {
      heat[i]=heat[i]-cooldown;
    }
  }
  
  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for( int k= num_pixels - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }
    
  // Step 3.  Randomly ignite new 'sparks' near the bottom
  if( random(255) < Sparking ) {
    int y = random(7);
    heat[y] = heat[y] + random(160,255);
    //heat[y] = random(160,255);
  }

  // Step 4.  Convert heat to LED colors
  for( int j = 0; j < num_pixels; j++) {
    setPixelHeatColor(j, heat[j] );
  }

  showStrip();
  delay(SpeedDelay);
}
*/