#include "serialize.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>

#define FIELD(json_field, structure_field)\
    if (json.find(json_field) != json.end()) structure_field = json[json_field]

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
    FIELD("hasSleepButton", hasSleepButton).toInt();
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
    return true;
}


