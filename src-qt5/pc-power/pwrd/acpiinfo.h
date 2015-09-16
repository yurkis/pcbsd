#ifndef ACPIINFO_H
#define ACPIINFO_H

#include "pwrd.h"

bool getBatteryInfo(int batt, PWRBatteryHardware& hwout, PWRSuppllyInfo &info);

#endif // ACPIINFO_H

