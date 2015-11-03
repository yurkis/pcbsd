#include "protocol.h"
#include <QJsonDocument>


QString QJsonObjectToMessage(QJsonObject obj)
{
    return QJsonObject2String(obj).replace('\n',' ');
}
