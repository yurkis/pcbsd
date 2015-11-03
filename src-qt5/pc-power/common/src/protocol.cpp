#include "protocol.h"
#include <QJsonDocument>


QString QJsonObjectToMessage(QJsonObject obj)
{
    QString ret;
    QString str = QJsonObject2String(obj);
    int length = str.length();
    for (int i=0; i<length; i++)
    {
        if ( (str[i] == ' ') || (str[i] == '\n') ) continue;
        ret+=str[i];
    }
    return ret;
    //return QJsonObject2String(obj).replace("\n"," ");
}
