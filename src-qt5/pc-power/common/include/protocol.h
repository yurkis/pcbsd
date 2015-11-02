#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QString>
#include <QJsonObject>

#include "serialize.h"

#define _str_constant static const char* const

//_str_constant MSG_TYPE_NAME = "msgtype";
_str_constant MSGTYPE_COMMAND = "command";
_str_constant MSGTYPE_DATA = "data";
_str_constant MSGTYPE_EVENT = "event";

_str_constant MSG_RESULT = "result";
_str_constant MSG_RESULT_SUCCESS = "success";
_str_constant MSG_RESULT_FAIL = "fail";
_str_constant MSG_RESULT_FAIL_REASON = "reason";

_str_constant COMMAND_HWINFO = "hwinfo";

_str_constant COMMAND_GET_BACKLIGHT = "getbacklight";

_str_constant COMMAND_SET_BACKLIGHT = "setbacklight";
_str_constant BACKLIGHT_NUMBER = "bl";
_str_constant BACKLIGHT_LEVELS = "levels";
_str_constant BACKLIGHT_VALUE = "val";

_str_constant COMMAND_ACTIVE_PROFILES = "getactiveprofiles";
_str_constant ON_AC_POWER_PROFILE_ID = "on_ac_id";
_str_constant ON_AC_POWER_PROFILE_NAME = "on_ac_name";
_str_constant ON_BATTERY_PROFILE_ID = "on_batt_id";
_str_constant ON_BATTERY_PROFILE_NAME = "on_batt_name";
_str_constant ON_LOW_BATTERY_PROFILE_ID = "on_low_batt_id";
_str_constant ON_LOW_BATTERY_PROFILE_NAME = "on_low_batt_name";

QString QJsonObjectToMessage(QJsonObject obj);


#endif // PROTOCOL_H

