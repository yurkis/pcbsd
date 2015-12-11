/**************************************************************************
*   Copyright (C) 2015- by Yuri Momotyuk                                   *
*   yurkis@pcbsd.org                                                      *
*                                                                         *
*   Permission is hereby granted, free of charge, to any person obtaining *
*   a copy of this software and associated documentation files (the       *
*   "Software"), to deal in the Software without restriction, including   *
*   without limitation the rights to use, copy, modify, merge, publish,   *
*   distribute, sublicense, and/or sell copies of the Software, and to    *
*   permit persons to whom the Software is furnished to do so, subject to *
*   the following conditions:                                             *
*                                                                         *
*   The above copyright notice and this permission notice shall be        *
*   included in all copies or substantial portions of the Software.       *
*                                                                         *
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       *
*   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    *
*   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*
*   IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR     *
*   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, *
*   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR *
*   OTHER DEALINGS IN THE SOFTWARE.                                       *
***************************************************************************/

#include "battery.h"

#include <dev/acpica/acpiio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <QDebug>

#ifndef FAKE_BATT

///////////////////////////////////////////////////////////////////////////////
static const char* const ACPIDEV = "/dev/acpi";
#define UNKNOWN_CAP 0xffffffff
#define UNKNOWN_VOLTAGE 0xffffffff


///////////////////////////////////////////////////////////////////////////////
bool getBatteryHWInfo(int batt, PWRBatteryHardware &hwout/*, PWRBatteryStatus &info*/)
{
    static int      acpifd;

    // Open ACPI device
    acpifd = open(ACPIDEV, O_RDWR);
    if (acpifd == -1)
        acpifd = open(ACPIDEV, O_RDONLY);
    if (acpifd == -1)
        return false;

    hwout = PWRBatteryHardware();

    //Make IOCTL call
    union acpi_battery_ioctl_arg battio;
    battio.unit = batt;
    if (ioctl(acpifd, ACPIIO_BATT_GET_BIF, &battio) == -1)
        return false;

    // Get battery H/W info
    hwout.OEMInfo = QString(battio.bif.oeminfo);
    hwout.model = QString (battio.bif.model);
    hwout.serial = QString (battio.bif.serial);
    hwout.type = QString(battio.bif.type);
    hwout.designCapacity = (battio.bif.dcap != UNKNOWN_CAP)?battio.bif.dcap:0;
    hwout.lastFullCapacity = (battio.bif.lfcap != UNKNOWN_CAP)?battio.bif.lfcap:0;
    hwout.designVoltage = (battio.bif.dvol != UNKNOWN_CAP)? battio.bif.dvol:0;

    // If units are in mAh - recalculate in mWh
    if (battio.bif.units == ACPI_BIF_UNITS_MA)
    {
        //TODO: CHECK!
        hwout.designCapacity= hwout.designCapacity * hwout.designVoltage / 100;
        hwout.lastFullCapacity = hwout.lastFullCapacity * hwout.designVoltage / 100;
    }

    /*
    // Get current power consumption
    info.powerConsumption = 0;

    if (battio.battinfo.rate >=0 )
    {   if ((battio.bif.units == ACPI_BIF_UNITS_MA) && battio.bst.volt != UNKNOWN_VOLTAGE)
        {
            // If units are in mAh
            info.powerConsumption = battio.bst.volt * battio.battinfo.rate / 100 ;
        }
        else
        {
            info.powerConsumption = battio.battinfo.rate;
        }
    }

    info.batteryState = BATT_STATE_UNKNOWN;
    switch (battio.bst.state)
    {
        case ACPI_BATT_STAT_CHARGING:
            info.batteryState = BATT_CHARGING;
            break;
        case ACPI_BATT_STAT_DISCHARG:
            info.batteryState = BATT_DISCHARGING;
            break;
        case ACPI_BATT_STAT_CRITICAL:
            info.batteryState = BATT_CRITICAL;
            break;
    };

    info.batteryRate = battio.bst.rate;

    //info.batteryRate = (battio.battinfo.cap>=0)?battio.battinfo.cap:0;
*/

    // Close ACPI device
    close(acpifd);

    return true;
}

///////////////////////////////////////////////////////////////////////////////
bool getBatteryStatus(int batt, PWRBatteryStatus& info)
{
    static int      acpifd;
    info = PWRBatteryStatus();

    // Open ACPI device
    acpifd = open(ACPIDEV, O_RDWR);
    if (acpifd == -1)
        acpifd = open(ACPIDEV, O_RDONLY);
    if (acpifd == -1)
        return false;

    union acpi_battery_ioctl_arg battio;
    battio.unit = batt;
    if (ioctl(acpifd, ACPIIO_BATT_GET_BATTINFO, &battio) == -1)
        return false;

    info.batteryCapacity = battio.battinfo.cap;

    info.batteryState = BATT_STATE_UNKNOWN;
    switch (battio.battinfo.state & ACPI_BATT_STAT_BST_MASK)
    {
        case ACPI_BATT_STAT_CHARGING:
            info.batteryState = BATT_CHARGING;
            break;
        case ACPI_BATT_STAT_DISCHARG:
            info.batteryState = BATT_DISCHARGING;
            break;
        case ACPI_BATT_STAT_CRITICAL:
            info.batteryState = BATT_DISCHARGING;//BATT_CRITICAL;
            break;
    };

    info.batteryTime = (battio.battinfo.min != -1)?battio.battinfo.min:0;


    // Get current power consumption
    info.powerConsumption = 0;

    if (battio.battinfo.rate >=0 )
    {
        if ((battio.bif.units == ACPI_BIF_UNITS_MA) && battio.bst.volt != UNKNOWN_VOLTAGE)
        {
            // If units are in mAh
            info.powerConsumption = battio.bst.volt * battio.battinfo.rate / 100 ;
         }
         else
         {
             info.powerConsumption = battio.battinfo.rate;
         }
    }

    // Close ACPI device
    close(acpifd);

    return true;
}
#endif
