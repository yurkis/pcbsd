#ifndef SERIALIZE_H
#define SERIALIZE_H

#include "pwrdtypes.h"

#include <QJsonObject>
#include <QJsonArray>

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
    virtual void toJSON(QJsonObject &json);
    virtual bool fromJSON(const QJsonObject &json);
}JSONProfile;


#endif // SERIALIZE_H

