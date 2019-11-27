#pragma once

#include "stdint.h"
#include "main.h"

typedef enum{
    equal_color = 1,
    Running_Lights,
    colorWipe
}pattern_effect_enum;

esp_err_t pattern_init();
void pattern_set_pixel_number(uint16_t num_pixels);
void pattern_set_effect(pattern_effect_enum pattern_effect_cmd, uint16_t *rgb_cmd, int delay_cmd);
