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

QString QJsonObjectToMessage(QJsonObject obj);
/*


QString MakeProtocolJSON(QString MsgTypeName, QString MsgTypeValue, QJsonObject payload = QJsonObject());
QString MakeProtocolJSON(QString MsgTypeName, QJsonObject payload);

QString MakeCommandJSON(QString command_name, QJsonObject payload = QJsonObject());

typedef struct PWRProtocolMessage:public JSONSerializer
{
    QString msgtype;
    QJsonObject payload;

    void addPayload(JSONSerializer object);

    virtual void toJSON(QJsonObject &json);
    virtual bool fromJSON(QJsonObject &json);
}PWRProtocolMessage;

*/

#endif // PROTOCOL_H

