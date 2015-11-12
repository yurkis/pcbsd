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

//get hardware info command
_str_constant COMMAND_HWINFO = "hwinfo";

_str_constant COMMAND_GET_BACKLIGHT = "getbacklight";

//set backlight command
_str_constant COMMAND_SET_BACKLIGHT = "setbacklight";
_str_constant BACKLIGHT_NUMBER = "bl";
_str_constant BACKLIGHT_LEVELS = "levels";
_str_constant BACKLIGHT_VALUE = "val";

//get active  profiles command
_str_constant COMMAND_ACTIVE_PROFILES = "getactiveprofiles";
_str_constant ON_AC_POWER_PROFILE_ID = "on_ac_id";
_str_constant ON_AC_POWER_PROFILE_NAME = "on_ac_name";
_str_constant ON_BATTERY_PROFILE_ID = "on_batt_id";
_str_constant ON_BATTERY_PROFILE_NAME = "on_batt_name";
_str_constant ON_LOW_BATTERY_PROFILE_ID = "on_low_batt_id";
_str_constant ON_LOW_BATTERY_PROFILE_NAME = "on_low_batt_name";
/* EXAMPLE:
 * "{     "command": "getactiveprofiles" } "
 =================
 * "{     "on_ac_id": "balanced_ac",     "on_ac_name": "AC power balanced",     "on_batt_id": "balanced_batt",     "on_batt_name": "Battery balanced",     "on_low_batt_id": "low_batt",     "on_low_batt_name": "Battery low power",     "result": "success" } "
*/

//get all profiles
_str_constant COMMAND_GET_PROFILES = "getprofiles";
_str_constant PROFILES_ARRAY = "profiles";
_str_constant PROFILE_ID = "profile_id";
_str_constant PROFILE_NAME = "profile_name";
/* EXAMPLE
 * "{     "command": "getprofiles" } "
 =================
 * "{     "profiles": [         {             "profile_id": "balanced_ac",             "profile_name": "AC power balanced"         },         {             "profile_id": "balanced_batt",             "profile_name": "Battery balanced"         },         {             "profile_id": "default",             "profile_name": ""         },         {             "profile_id": "desktop",             "profile_name": "Desktop like. Full power."         },         {             "profile_id": "low_batt",             "profile_name": "Battery low power"         }     ],     "result": "success" } "
*/

//get profile
_str_constant COMMAND_GET_PROFILE = "getprofile";
//using PROFILE_ID, if not present - current profile shown

//current profile
_str_constant COMMAND_GET_CURRENT_PROFILE = "getcurrprofile";

_str_constant COMMAND_AC_STATUS = "getacstatus";
_str_constant AC_POWER = "acpower";

_str_constant COMMAND_GET_BATT_STATE = "getbattstate";

_str_constant COMMAND_SET_ACPI_STATE = "setacpistate";
_str_constant ACPI_STATE = "satte";

QString QJsonObjectToMessage(QJsonObject obj);



_str_constant EVENT_EVENT_FIELD = "event";

_str_constant EVENT_BACKLIGHT_CHANGED = "backlightchanged";
_str_constant EVENT_BATT_RATE_CHANGED = "battratechanged";
_str_constant EVENT_BATT_STATE_CHANGED = "battstatechanged";
_str_constant BATTERY_NO = "batt";


#endif // PROTOCOL_H

