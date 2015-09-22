#ifndef BATTERY_H
#define BATTERY_H

#include "pwrd.h"

bool getBatteryHWInfo(int batt, PWRBatteryHardware& hwout, PWRSuppllyInfo &info);
bool isOnBattery();

#endif // ACPIINFO_H

