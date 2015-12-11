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
\brief PWRD settings (pwrd.conf) reader / writer
*/


#ifndef _SETINGSREADER_H
#define _SETINGSREADER_H

#include <QString>
#include "serialize.h"

static const char* const DEF_CONFIG_FILE = "/usr/local/share/pcbsd/pwrd/pwrd.conf";
static const char* const DEF_ON_BATTERY_PROFILE_ID = "balanced_batt";
static const char* const DEF_ON_AC_POWER_PROFILE_ID = "balanced_ac";
static const char* const DEF_ON_LOW_POWER_PROFILE_ID = "low_batt";

typedef struct _PWRServerSettings:public JSONDaemonSettings
{
    QString pipeName;
    QString eventsPipeName;
    QString devdPipeName;
    QString profilesPath;

    unsigned int battPollingTime;

    _PWRServerSettings();
    bool load(QString file = DEF_CONFIG_FILE);
    bool save(QString file = DEF_CONFIG_FILE);

}PWRServerSettings;

#endif
