#ifndef BACKLIGHT_H
#define BACKLIGHT_H

#include "pwrdtypes.h"

bool getBacklightHWInfo(int num, PWRBacklightHardware& out);
bool setBacklightLevel(int num, int percentage);
int backlightLevel(int num);

#endif // BACKLIGHT_H

