#ifndef _SETINGSREADER_H
#define _SETINGSREADER_H

#include <QString>

static const char* const DEF_CONFIG_FILE = "/usr/local/share/pcbsd/pwrd/pwrd.conf";

typedef struct _PWRServerSettings
{
    QString pipeName;
    QString devdPipeName;
    QString profilesPath;
    unsigned int battPollingTime;

    _PWRServerSettings();
    bool load(QString file = DEF_CONFIG_FILE);

}PWRServerSettings;

#endif
