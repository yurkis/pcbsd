#include "serialize.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>


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

bool JSONHWInfo::fromJSON(QJsonObject &json)
{

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

bool JSONBatteryHardware::fromJSON(QJsonObject &json)
{

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

bool JSONBacklightHardware::fromJSON(QJsonObject &json)
{

}


