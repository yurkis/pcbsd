#ifndef SERIALIZE_H
#define SERIALIZE_H

#include "pwrdtypes.h"

#include <QJsonObject>
#include <QJsonArray>

typedef struct JSONSerializer
{
    virtual void toJSON(QJsonObject &json)=0;
    virtual bool fromJSON(QJsonObject &json)=0;

    QString toJSONString();

}JSONSerializer;

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

QString QJsonObject2String(QJsonObject obj);

typedef struct JSONHWInfo: public PWRHWInfo, JSONSerializer
{
    virtual void toJSON(QJsonObject &json);
    virtual bool fromJSON(QJsonObject &json);
}JSONHWInfo;

typedef struct JSONBatteryHardware: public PWRBatteryHardware, JSONSerializer
{
    virtual void toJSON(QJsonObject &json);
    virtual bool fromJSON(QJsonObject &json);
}JSONBatteryHardware;

typedef struct JSONBacklightHardware: public PWRBacklightHardware, JSONSerializer
{
    virtual void toJSON(QJsonObject &json);
    virtual bool fromJSON(QJsonObject &json);
}JSONBacklightHardware;


#endif // SERIALIZE_H

