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

#include "profilereader.h"

#include <QSettings>
#include <QFile>
#include <QDebug>

///////////////////////////////////////////////////////////////////////////////
#define _str_constant static const char* const

const unsigned int DEF_BACKLIGHT_LEVEL = 90;
_str_constant DEF_POWER_BTN_STATE = "S5";
_str_constant DEF_SLEEP_BTN_STATE = "S3";
_str_constant DEF_LID_STATE = "S3";
_str_constant DEF_NAME = "Unnamed or default profile";

_str_constant GENERAL_GROUP = "";//"general";
_str_constant NAME_FIELD = "id";
_str_constant DESCRIPTION_FIELD = "description";

_str_constant BUTTONS_GROUP = "buttons";
_str_constant LID_FIELD = "lid";
_str_constant SLEEP_FIELD = "sleep";
_str_constant POWER_FIELD = "power";

_str_constant LCD_GROUP = "lcd";
_str_constant BACKLIGHT_FIELD = "backlight";


///////////////////////////////////////////////////////////////////////////////
_PWRProfileReader::_PWRProfileReader()
{
    id = DEF_PROFILE_ID;
    description = DEF_NAME;
    btnPowerSate = DEF_POWER_BTN_STATE;
    btnSleepSate = DEF_SLEEP_BTN_STATE;
    lidSwitchSate = DEF_LID_STATE;
    lcdBrightness = DEF_BACKLIGHT_LEVEL;
}

///////////////////////////////////////////////////////////////////////////////
bool _PWRProfileReader::read(QString file)
{
    if (!QFile::exists(file))
        return false;

    QSettings Reader(file, QSettings::IniFormat);
        if (Reader.status() != QSettings::NoError)
            return false;
    Reader.setIniCodec("UTF-8");

    Reader.beginGroup(GENERAL_GROUP);
    id = Reader.value(NAME_FIELD, QString(DEF_PROFILE_ID)).toString();
    description = Reader.value(DESCRIPTION_FIELD, QString(DEF_NAME)).toString();
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

///////////////////////////////////////////////////////////////////////////////
bool _PWRProfileReader::write(QString file)
{
    qDebug()<<"Writing profile "<<file;

    QSettings Writer(file, QSettings::IniFormat);
        if (Writer.status() != QSettings::NoError)
            return false;
    Writer.setIniCodec("UTF-8");

    Writer.beginGroup(GENERAL_GROUP);
    Writer.setValue(NAME_FIELD, id);
    Writer.setValue(DESCRIPTION_FIELD, description);
    Writer.endGroup();

    Writer.beginGroup(BUTTONS_GROUP);
    Writer.setValue(LID_FIELD, lidSwitchSate);
    Writer.setValue(SLEEP_FIELD, btnSleepSate);
    Writer.setValue(POWER_FIELD, btnPowerSate);
    Writer.endGroup();

    Writer.beginGroup(LCD_GROUP);
    Writer.setValue(BACKLIGHT_FIELD, lcdBrightness);
    Writer.endGroup();

    Writer.sync();

    return true;
}
