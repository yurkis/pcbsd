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

#include "serialize.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>

#define FIELD(json_field, structure_field)\
    if (json.find(json_field) != json.end()) structure_field = json[json_field]

#define FIELD_REQUIRED(json_field, structure_field)\
    if (!json.contains(json_field)) return false;\
        else structure_field = json[json_field];

QString QJsonObject2String(QJsonObject obj)
{
    QJsonDocument doc (obj);
    return doc.toJson();
}

QString JSONSerializer::toJSONString()
{
    QJsonObject obj;
    toJSON(obj);
    return QJsonObject2String(obj);
}

QJsonObject JSONSerializer::toJSON()
{
    QJsonObject obj;
    toJSON(obj);
    return obj;
}

void JSONHWInfo::toJSON(QJsonObject &json)
{
    json["numBatteries"] = numBatteries;
    json["numBacklights"] = numBacklights;
    json["hasSleepButton"] = hasSleepButton;
    json["hasLid"] = hasLid;

    QJsonArray arr;
    for(int i=0; i<possibleACPIStates.size(); i++)
    {
        arr.append(QJsonValue(possibleACPIStates[i]));
    }
    json["possibleACPIStates"] = arr;
}

bool JSONHWInfo::fromJSON(const QJsonObject &json)
{
    FIELD("numBatteries", numBatteries).toInt();
    FIELD("numBacklights", numBacklights).toInt();
    FIELD("hasSleepButton", hasSleepButton).toBool();
    FIELD("hasLid", hasLid).toBool();
    QJsonArray arr;
    possibleACPIStates.clear();
    if (json.find("possibleACPIStates") != json.end())
    {
        arr = json["possibleACPIStates"].toArray();
        for(int i=0; i<arr.size(); i++)
        {
            possibleACPIStates<<arr[i].toString();
        }
    }
    return true;
}

void JSONBatteryHardware::toJSON(QJsonObject &json)
{
    json["OEMInfo"] = OEMInfo;
    json["model"] = model;
    json["serial"] = serial;
    json["type"] = type;
    json["designCapacity"] = (int)designCapacity;
    json["lastFullCapacity"] = (int)lastFullCapacity;
    json["designVoltage"] = (int)designVoltage;
}

bool JSONBatteryHardware::fromJSON(const QJsonObject &json)
{
    FIELD("OEMInfo", OEMInfo).toString();
    FIELD("model", model).toString();
    FIELD("serial", serial).toString();
    FIELD("type", type).toString();
    FIELD("designCapacity", designCapacity).toInt();
    FIELD("lastFullCapacity", lastFullCapacity).toInt();
    FIELD("designVoltage", designVoltage).toInt();
    return true;
}

void JSONBacklightHardware::toJSON(QJsonObject &json)
{
    QJsonArray arr;
    for (int i=0; i<backlightLevels.size(); i++)
    {
        arr.append(QJsonValue((int)backlightLevels[i]));
    }
    json["backlightLevels"] = arr;
}

bool JSONBacklightHardware::fromJSON(const QJsonObject &json)
{
    Q_UNUSED(json);
    return true;
}

#define CC_FIELD(field)\
    field = in.field;

JSONProfile::JSONProfile(const PWRProfile &in)
{
    CC_FIELD(id);
    CC_FIELD(description);
    CC_FIELD(btnPowerSate);
    CC_FIELD(btnSleepSate);
    CC_FIELD(lidSwitchSate);
    CC_FIELD(lcdBrightness);
}

void JSONProfile::toJSON(QJsonObject &json)
{
    json["id"] = id;
    json["description"] = description;
    json["btnPowerSate"] = btnPowerSate;
    json["btnSleepSate"] = btnSleepSate;
    json["lidSwitchSate"] = lidSwitchSate;
    json["lcdBrightness"] = (int)lcdBrightness;
}

bool JSONProfile::fromJSON(const QJsonObject &json)
{
    FIELD("id", id).toString();
    FIELD("description", description).toString();
    FIELD("btnPowerSate", btnPowerSate).toString();
    FIELD("btnSleepSate", btnSleepSate).toString();
    FIELD("lidSwitchSate", lidSwitchSate).toString();
    FIELD("lcdBrightness", lcdBrightness).toInt();
    return true;
}

/*    PWRBatteryState batteryState;   //< Current battery state
    unsigned int batteryRate;       //< Battery rate in percents (0..100)
    unsigned int powerConsumption;  //< Current power consumption (in mW)
    unsigned int batteryTime;       //< Battery lifetime (in minutes)*/

void JSONBatteryStatus::toJSON(QJsonObject &json)
{
    json["batteryCapacity"] = (int)batteryCapacity;
    json["powerConsumption"] = (int)powerConsumption;
    json["batteryTime"] = (int)batteryTime;
    json["batteryCritical"]=batteryCritical;

    QString state=UNKNOWN;
    switch(batteryState)
    {
        case BATT_CHARGING:
            state = CHARGING;
            break;
        case BATT_DISCHARGING:
            state = DISCHARGING;
            break;
        default:
            state = UNKNOWN;
    }
    json["batteryState"] = state;
}

bool JSONBatteryStatus::fromJSON(const QJsonObject &json)
{
    FIELD("batteryCapacity", batteryCapacity).toInt();
    FIELD("powerConsumption", powerConsumption).toInt();
    FIELD("batteryTime", batteryTime).toInt();
    FIELD("batteryCritical", batteryCritical).toBool();
    batteryState = BATT_STATE_UNKNOWN;
    if (json.contains("batteryState"))
    {
        if (json["batteryState"].toString() == CHARGING) batteryState = BATT_CHARGING;
        else if (json["batteryState"].toString() == DISCHARGING) batteryState = BATT_DISCHARGING;
    }
    return true;
}

void _JSONDaemonSettings::toJSON(QJsonObject &json)
{
    json["lowBatteryCapacity"] = lowBatteryCapacity;
    json["onACProfile"] = onACProfile;
    json["onBatteryProfile"] = onBatteryProfile;
    json["onLowBatteryProfile"] = onLowBatteryProfile;
    json["usingIntel_backlight"] = usingIntel_backlight;
    json["allowSettingsChange"] = allowSettingsChange;
    json["allowProfileChange"] = allowProfileChange;
}

bool _JSONDaemonSettings::fromJSON(const QJsonObject &json)
{
    FIELD("lowBatteryCapacity", lowBatteryCapacity).toInt();
    FIELD("onACProfile", onACProfile).toString();
    FIELD("onBatteryProfile", onBatteryProfile).toString();
    FIELD("onLowBatteryProfile", onLowBatteryProfile).toString();
    FIELD("usingIntel_backlight", usingIntel_backlight).toBool();
    FIELD("allowSettingsChange", allowSettingsChange).toBool();
    FIELD("allowProfileChange", allowProfileChange).toBool();

    return true;
}
