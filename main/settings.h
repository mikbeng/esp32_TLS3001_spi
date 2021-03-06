#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>

typedef struct {
	uint16_t universeStart;
    uint16_t universeStop;
    uint16_t dmx_start;
    char ssid[32];  
    char password[64];
} settings_t;

settings_t settings;

void LoadSettings();
void SaveSettings();

#endif /* SETTINGS_H */

