#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QString>
#include <QJsonObject>

#include "serialize.h"

#define _str_constant static const char* const

_str_constant MSG_TYPE_NAME = "msgtype";
_str_constant MSGTYPE_COMMAND = "command";
_str_constant MSGTYPE_DATA = "data";
_str_constant MSGTYPE_EVENT = "event";

_str_constant COMMAND_HWINFO = "hwinfo";
_str_constant COMMAND_GET_BACKLIGHT = "getbacklight";
_str_constant COMMAND_SET_BACKLIGHT = "setbacklight";
_str_constant BACKLIGHT_NUMBER = "bl";
_str_constant BACKLIGHT_LEVELS = "levels";
_str_constant BACKLIGHT_VALUE = "val";

QString QJsonObjectToMessage(QJsonObject obj);


#endif // PROTOCOL_H

