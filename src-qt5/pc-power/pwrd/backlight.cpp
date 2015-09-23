#include "backlight.h"
#include "sysctlutils.h"
#include <QDebug>

#define _str_constant static const char* const
_str_constant DEF_BAKLIGHT_ENABLED_SYSCTL = "hw.acpi.video.lcd%1.active";
_str_constant DEF_BAKLIGHT_LEVELS_SYSCTL = "hw.acpi.video.lcd%1.levels";

bool getBacklightHWInfo(int num, PWRBacklightHardware &out)
{
    out = PWRBacklightHardware();
    //TODO: May be not only lcd0. Should rework it later!
    if (!sysctlPresent(QString(DEF_BAKLIGHT_ENABLED_SYSCTL).arg(num)))
        return false;
    if (!sysctlAsInt(QString(DEF_BAKLIGHT_ENABLED_SYSCTL).arg(num)))
        return false;

    QStringList levels = sysctl(QString(DEF_BAKLIGHT_LEVELS_SYSCTL).arg(num)).split(" ");

    for(int i=0; i<levels.size(); i++)
    {
        bool found = false;
        unsigned int val = levels[i].toInt();

        // Dont save duplicates
        for(int j=0; j<out.backlightLevels.size(); j++)
        {
            if (val == out.backlightLevels[j])
            {
                found = true;
                break;
            }            
        }
        if (!found) out.backlightLevels.push_back(val);
    }
    return true;
}
