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

}PWRServerSettings;

#endif
