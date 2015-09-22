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

///////////////////////////////////////////////////////////////////////////////
#define _str_constant static const char* const

_str_constant DEF_PIPE_NAME = "/var/run/pwrd.pipe";
_str_constant DEF_PROFILES_PATH = "/usr/local/share/pcbsd/pwrd/profiles/";
static const int DEF_POLLING_BATTERY_TIME = 1000;
_str_constant DEF_DEVD_PIPE = "/var/run/devd.pipe";

_str_constant CONF_FIELD_PIPE_NAME = "pipe";
_str_constant CONF_FIELD_DEVD_PIPE = "devd_pipe";
_str_constant CONF_FIELD_PROFILES_PATH = "profiles_path";
_str_constant CONF_FIELD_BATTER_POLLING = "battery_polling";

///////////////////////////////////////////////////////////////////////////////
_PWRServerSettings::_PWRServerSettings()
{
    pipeName = DEF_PIPE_NAME;
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

    pipeName = Reader.value(CONF_FIELD_PIPE_NAME, DEF_PIPE_NAME).toString();
    devdPipeName = Reader.value(CONF_FIELD_DEVD_PIPE, DEF_DEVD_PIPE).toString();
    profilesPath = Reader.value(CONF_FIELD_PROFILES_PATH, DEF_PROFILES_PATH).toString();
    battPollingTime =Reader.value(CONF_FIELD_BATTER_POLLING, DEF_POLLING_BATTERY_TIME).toInt();

    return true;
}
