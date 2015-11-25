/**************************************************************************
*   Copyright (C) 2015 by Yuri Momotyuk                                   *
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

#include "settingsreader.h"

#include <QSettings>
#include <QDebug>
#include <QFile>

#include "pwrdtypes.h"

///////////////////////////////////////////////////////////////////////////////
#define _str_constant static const char* const

_str_constant DEF_PROFILES_PATH = "/usr/local/share/pcbsd/pwrd/";
static const int DEF_POLLING_BATTERY_TIME = 1000;
_str_constant DEF_DEVD_PIPE = "/var/run/devd.pipe";

_str_constant CONF_FIELD_PIPE_NAME = "pipe";
_str_constant CONF_FIELD_DEVD_PIPE = "devd_pipe";
_str_constant CONF_FIELD_PROFILES_PATH = "profiles_path";
_str_constant CONF_FIELD_BATTER_POLLING = "battery_polling";
_str_constant CONF_FIELD_INTEL_BACKLIGHT = "use_intel_backlight";
_str_constant CONF_FIELD_LOW_BATTERY_RATE = "low_battery_rate";

_str_constant CONF_FIELD_ALLOW_CONFIG_CHANGE = "allow_config_change";
const int DEF_ALLOW_CONFIG_CHANGE = 1;
_str_constant CONF_FIELD_ALLOW_PROFILES_CHANGE = "allow_profiles_change";
const int DEF_ALLOW_PROFILES_CHANGE = 1;

_str_constant CONF_PROFILES_GROUP = "profiles";
_str_constant CONF_ON_AC_POWER = "on_ac_power";
_str_constant CONF_ON_BATTERY = "on_battery";
_str_constant CONF_ON_LOW_BATTERY = "on_low_battery";
_str_constant DEF_PROFILE = "";

const int DEF_FIELD_LOW_BATTERY_RATE = 10;

///////////////////////////////////////////////////////////////////////////////
_PWRServerSettings::_PWRServerSettings()
{
    pipeName = DEF_PWRD_PIPE_NAME;
    devdPipeName = DEF_DEVD_PIPE;
    profilesPath = DEF_PROFILES_PATH;
    battPollingTime = DEF_POLLING_BATTERY_TIME;
}

///////////////////////////////////////////////////////////////////////////////
bool _PWRServerSettings::load(QString file)
{
    if (!QFile::exists(file))
        return false;

    QSettings Reader(file, QSettings::IniFormat);
        if (Reader.status() != QSettings::NoError)
            return false;
    Reader.setIniCodec("UTF-8");

    pipeName = Reader.value(CONF_FIELD_PIPE_NAME, DEF_PWRD_PIPE_NAME).toString();
    eventsPipeName = Reader.value(CONF_FIELD_PIPE_NAME, DEF_PWRD_EVENTS_PIPE_NAME).toString();
    devdPipeName = Reader.value(CONF_FIELD_DEVD_PIPE, DEF_DEVD_PIPE).toString();
    profilesPath = Reader.value(CONF_FIELD_PROFILES_PATH, DEF_PROFILES_PATH).toString();
    battPollingTime =Reader.value(CONF_FIELD_BATTER_POLLING, DEF_POLLING_BATTERY_TIME).toInt();
    lowBatteryCapacity = Reader.value(CONF_FIELD_LOW_BATTERY_RATE, DEF_FIELD_LOW_BATTERY_RATE).toInt();
    usingIntel_backlight =  Reader.value(CONF_FIELD_INTEL_BACKLIGHT, 0).toInt() != 0;
    allowSettingsChange = Reader.value(CONF_FIELD_ALLOW_CONFIG_CHANGE, DEF_ALLOW_CONFIG_CHANGE).toInt() != 0;
    allowProfileChange = Reader.value(CONF_FIELD_ALLOW_PROFILES_CHANGE, DEF_ALLOW_PROFILES_CHANGE).toInt() != 0;

    Reader.beginGroup(CONF_PROFILES_GROUP);
    onBatteryProfile = Reader.value(CONF_ON_BATTERY, DEF_PROFILE).toString();
    onACProfile = Reader.value(CONF_ON_AC_POWER, DEF_PROFILE).toString();
    onLowBatteryProfile = Reader.value(CONF_ON_LOW_BATTERY, DEF_PROFILE).toString();

    return true;
}

bool _PWRServerSettings::save(QString file)
{
    QSettings Writer(file, QSettings::IniFormat);
        if (Writer.status() != QSettings::NoError)
            return false;
    Writer.setIniCodec("UTF-8");

    Writer.beginGroup("general");

    Writer.setValue(CONF_FIELD_PIPE_NAME, pipeName);
    Writer.setValue(CONF_FIELD_PIPE_NAME, eventsPipeName);
    Writer.setValue(CONF_FIELD_DEVD_PIPE, devdPipeName);
    Writer.setValue(CONF_FIELD_PROFILES_PATH, profilesPath);
    Writer.setValue(CONF_FIELD_BATTER_POLLING, battPollingTime);
    Writer.setValue(CONF_FIELD_LOW_BATTERY_RATE, lowBatteryCapacity);
    Writer.setValue(CONF_FIELD_INTEL_BACKLIGHT, (usingIntel_backlight)?1:0);
    Writer.setValue(CONF_FIELD_ALLOW_CONFIG_CHANGE, allowSettingsChange);
    Writer.setValue(CONF_FIELD_ALLOW_PROFILES_CHANGE, allowProfileChange);

    Writer.beginGroup(CONF_PROFILES_GROUP);
    Writer.setValue(CONF_ON_AC_POWER, onACProfile);
    Writer.setValue(CONF_ON_BATTERY, onBatteryProfile);
    Writer.setValue(CONF_ON_LOW_BATTERY, onLowBatteryProfile);

    return true;
}
