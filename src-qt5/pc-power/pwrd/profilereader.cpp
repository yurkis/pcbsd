#include "profilereader.h"

#include <QSettings>
#include <QFile>
#include <QDebug>


#define _str_constant static const char* const

const unsigned int DEF_BACKLIGHT_LEVEL = 90;
_str_constant DEF_POWER_BTN_STATE = "S5";
_str_constant DEF_SLEEP_BTN_STATE = "S3";
_str_constant DEF_LID_STATE = "S3";
_str_constant DEF_NAME = "Default";

_str_constant GENERAL_GROUP = "";//"general";
_str_constant NAME_FIELD = "name";
_str_constant DESCRIPTION_FIELD = "description";

_str_constant BUTTONS_GROUP = "buttons";
_str_constant LID_FIELD = "lid";
_str_constant SLEEP_FIELD = "sleep";
_str_constant POWER_FIELD = "power";

_str_constant LCD_GROUP = "lcd";
_str_constant BACKLIGHT_FIELD = "backlight";

/*
 *
 *     QString name;
    QString description;
    QString btnPowerSate;
    QString btnSleepSate;
    QString lidSwitchSate;
    unsigned int lcdBrightness;
    */

_PWRProfileReader::_PWRProfileReader()
{
    name = DEF_NAME;
    btnPowerSate = DEF_POWER_BTN_STATE;
    btnSleepSate = DEF_SLEEP_BTN_STATE;
    lidSwitchSate = DEF_LID_STATE;
    lcdBrightness = DEF_BACKLIGHT_LEVEL;
}

bool _PWRProfileReader::read(QString file)
{
    if (!QFile::exists(file))
        return false;

    QSettings Reader(file, QSettings::IniFormat);
        if (Reader.status() != QSettings::NoError)
            return false;
    Reader.setIniCodec("UTF-8");

    Reader.beginGroup(GENERAL_GROUP);
    name = Reader.value(NAME_FIELD, QString(DEF_NAME)).toString();
    description = Reader.value(DESCRIPTION_FIELD).toString();
    Reader.endGroup();

    Reader.beginGroup(BUTTONS_GROUP);
    lidSwitchSate = Reader.value(LID_FIELD, QString()).toString();
    btnSleepSate = Reader.value(SLEEP_FIELD, QString(DEF_SLEEP_BTN_STATE)).toString();
    btnPowerSate = Reader.value(POWER_FIELD, QString(DEF_POWER_BTN_STATE)).toString();
    Reader.endGroup();

    Reader.beginGroup(LCD_GROUP);
    lcdBrightness = (unsigned int) Reader.value(BACKLIGHT_FIELD, DEF_BACKLIGHT_LEVEL).toInt();
    Reader.endGroup();

    return true;
}
