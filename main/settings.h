#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>

typedef struct {
	uint32_t universeStart;
} settings_t;

settings_t settings;

void LoadSettings();
void SaveSettings();

#endif /* SETTINGS_H */

