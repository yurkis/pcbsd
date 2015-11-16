
#ifndef BATTERY_H
#define BATTERY_H

#include "pwrdtypes.h"

//#define FAKE_BATT

bool getBatteryHWInfo(int batt, PWRBatteryHardware& hwout);
bool getBatteryStatus(int batt, PWRBatteryStatus& info);

#endif // ACPIINFO_H

