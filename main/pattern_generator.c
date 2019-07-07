#include "TLS3001.h"
#include "main.h"
#include "string.h"

uint16_t input_color_array[(num_pixels_max*3)];

/*
struct pixel
{
    uint16_t rgb[3];
    uint16_t intensity;
};

struct pixel pixels[num_pixels_max];
*/

void pattern_equal_color(void *spi_tx_data_start, uint16_t *rgb, uint16_t num_pixels)
{
    uint16_t red = *rgb;
    uint16_t green = *(rgb+1);
    uint16_t blue = *(rgb+2);

    for (size_t i = 0; i < num_pixels; i++)
	{
		input_color_array[(i*3)+0] = red;
		input_color_array[(i*3)+1] = green;
		input_color_array[(i*3)+2] = blue;
	}

    //prepare the color data
	TLE3001_prep_color_packet(spi_tx_data_start, &input_color_array, num_pixels);
}


void pattern_shift_pixels_right(void *spi_tx_data_start, uint16_t pixel_index_start, uint16_t pixel_index_end, uint16_t num_pixels)
{
    //Function requires you to have pixel data in *spi_tx_data_start. I.e by first setting pattern_equal_color() etc.

    uint16_t *pixel_pointer_start = &input_color_array + (pixel_index_start*3);
    
}