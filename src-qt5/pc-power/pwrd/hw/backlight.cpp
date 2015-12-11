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

/*!
\file
\brief Backlight related functions. This covers sysctl based backlight control
*/

#include "backlight.h"
#include "sysctlutils.h"
#include <QDebug>

#define _str_constant static const char* const
_str_constant DEF_BAKLIGHT_ENABLED_SYSCTL = "hw.acpi.video.lcd%1.active";
_str_constant DEF_BAKLIGHT_LEVELS_SYSCTL = "hw.acpi.video.lcd%1.levels";
_str_constant LCD_BRIGHTNESS_SYSCTL = "hw.acpi.video.lcd%1.brightness";

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

bool setBacklightLevel(int num, int percentage)
{
    if (!sysctlPresent(QString(DEF_BAKLIGHT_ENABLED_SYSCTL).arg(num)))
         return false;
    return setSysctl(QString(LCD_BRIGHTNESS_SYSCTL).arg(num), percentage);
}

int backlightLevel(int num)
{
    if (!sysctlPresent(QString(LCD_BRIGHTNESS_SYSCTL).arg(num)))
        return false;
    return sysctlAsInt(QString(LCD_BRIGHTNESS_SYSCTL).arg(num));
}
