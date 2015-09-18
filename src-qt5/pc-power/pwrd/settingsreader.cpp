#include "settingsreader.h"

#include <QSettings>
#include <QDebug>
#include <QFile>

#define _str_constant static const char* const

_str_constant DEF_PIPE_NAME = "/var/run/pwrd.pipe";
_str_constant DEF_PROFILES_PATH = "/usr/local/share/pcbsd/pwrd/profiles/";
static const int DEF_POLLING_BATTERY_TIME = 1000;
_str_constant DEF_DEVD_PIPE = "/var/run/devd.pipe";

_str_constant CONF_FIELD_PIPE_NAME = "pipe";
_str_constant CONF_FIELD_DEVD_PIPE = "devd_pipe";
_str_constant CONF_FIELD_PROFILES_PATH = "profiles_path";
_str_constant CONF_FIELD_BATTER_POLLING = "battery_polling";


_PWRServerSettings::_PWRServerSettings()
{
    pipeName = DEF_PIPE_NAME;
    devdPipeName = DEF_DEVD_PIPE;
    profilesPath = DEF_PROFILES_PATH;
    battPollingTime = DEF_POLLING_BATTERY_TIME;
}

#define READ_STR_IF_PRESENT(FIELD_NAME, STRUCT_MEMBER)\
    Str = Reader.value(FIELD_NAME).toString();

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
