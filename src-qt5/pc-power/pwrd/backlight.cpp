#include "backlight.h"
#include "sysctlutils.h"

#define _str_constant static const char* const
_str_constant DEF_BAKLIGHT_ENABLED_SYSCTL = "hw.acpi.video.lcd0.active";

void getBacklightHWInfo(PWRBacklightHardware &out)
{
    out = PWRBacklightHardware();
    //TODO: May be not only lcd0. Should rework it later!
    out.hasBacklight = sysctlPresent(DEF_BAKLIGHT_ENABLED_SYSCTL);
    out.hasBacklight&= sysctlAsInt(DEF_BAKLIGHT_ENABLED_SYSCTL) == 1;
    if (!out.hasBacklight)
        return;

    QStringList levels = sysctl("hw.acpi.video.lcd0.levels").split(" ");

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
            if (!found) out.backlightLevels.push_back(val);
        }
    }
}
