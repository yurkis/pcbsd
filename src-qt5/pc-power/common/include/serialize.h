
/*!
\file
\brief PWRD data types serializers for JSON protocol
*/

#ifndef SERIALIZE_H
#define SERIALIZE_H

#include "pwrdtypes.h"

#include <QJsonObject>
#include <QJsonArray>

static const char* const CHARGING = "charging";
static const char* const DISCHARGING = "discharging";
static const char* const UNKNOWN = "unknown";

typedef struct JSONSerializer
{
public:
    virtual void toJSON(QJsonObject &json)=0;
    virtual bool fromJSON(const QJsonObject &json)=0;
    virtual QString myname()=0;

    QString toJSONString();
    virtual QJsonObject toJSON();

}JSONSerializer;

#define JSON_STRUCT(name)\
    virtual QString myname(){return name;};\
    using JSONSerializer::toJSON;   //that behavour surpise me :(

template <typename T>
void QVector2JSON(QString array_name, QVector<T> vec, QJsonObject &obj)
{
    QJsonArray arr;
    for(int i=0; i< vec.size(); i++)
    {
        QJsonObject obj;
        vec[i].toJSON(obj);
        arr.append(obj);
    }
    obj[array_name] = arr;
}

template <typename T>
QJsonObject QVector2JSON(QString array_name, QVector<T> vec)
{
    QJsonObject obj;
    QJsonArray arr;
    for(int i=0; i< vec.size(); i++)
    {
        QJsonObject obj;
        vec[i].toJSON(obj);
        arr.append(obj);
    }
    obj[array_name] = arr;
    return obj;
}

QString QJsonObject2String(QJsonObject obj);

typedef struct JSONHWInfo: public PWRHWInfo, public JSONSerializer
{
    JSON_STRUCT("HWInfo");
    virtual void toJSON(QJsonObject &json);
    virtual bool fromJSON(const QJsonObject &json);
}JSONHWInfo;

typedef struct JSONBatteryHardware: public PWRBatteryHardware, JSONSerializer
{
    JSON_STRUCT("BatteryHardware");
    virtual void toJSON(QJsonObject &json);
    virtual bool fromJSON(const QJsonObject &json);
}JSONBatteryHardware;

typedef struct JSONBacklightHardware: public PWRBacklightHardware, JSONSerializer
{
    JSON_STRUCT("BacklightHardware");
    virtual void toJSON(QJsonObject &json);
    virtual bool fromJSON(const QJsonObject &json);
}JSONBacklightHardware;

typedef struct JSONProfile:public PWRProfile, public JSONSerializer
{
    JSON_STRUCT("Profile");
    JSONProfile(){;};
    JSONProfile(const PWRProfile& in);
    virtual void toJSON(QJsonObject &json);
    virtual bool fromJSON(const QJsonObject &json);
}JSONProfile;

typedef struct _JSONBatteryStatus: public PWRBatteryStatus, public JSONSerializer
{
    JSON_STRUCT("BatteryStatus")
    virtual void toJSON(QJsonObject &json);
    virtual bool fromJSON(const QJsonObject &json);
}JSONBatteryStatus;

typedef struct _JSONDaemonSettings: public PWRDaemonSettings, JSONSerializer
{
    JSON_STRUCT("DaemonSettings")
    virtual void toJSON(QJsonObject &json);
    virtual bool fromJSON(const QJsonObject &json);
}JSONDaemonSettings;

#endif // SERIALIZE_H

