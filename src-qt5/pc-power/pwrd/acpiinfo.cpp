#include "acpiinfo.h"

#include <dev/acpica/acpiio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>

static const char* const ACPIDEV = "/dev/acpi";
#define UNKNOWN_CAP 0xffffffff
#define UNKNOWN_VOLTAGE 0xffffffff

static int      acpifd;


bool getBatteryInfo(int batt, PWRBatteryHardware &hwout, PWRSuppllyInfo& info)
{
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
    hwout.hasBattery = true;
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


    // Close ACPI device
    close(acpifd);

    return true;
}
