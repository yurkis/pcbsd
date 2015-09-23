#include "protocol.h"
#include <QJsonDocument>

/*
void PWRProtocolMessage::addPayload(JSONSerializer object)
{
    payload[object.myname()] = object.toJSON();
}

void PWRProtocolMessage::toJSON(QJsonObject &json)
{
    json[MSG_TYPE_NAME] = msgtype;
    json["payload"] = payload;
}
*/

QString QJsonObjectToMessage(QJsonObject obj)
{
    return QJsonObject2String(obj).replace("\n"," ");
}
