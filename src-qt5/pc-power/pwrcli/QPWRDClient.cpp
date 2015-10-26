#include "QPWRDClient.h"

#include <QLocalSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTextStream>

#include "serialize.h"
#include "protocol.h"

class QPWRDClientPrivate
{
public:
    QPWRDClientPrivate(QPWRDClient *parent)
        : q_ptr(parent){}

    QLocalSocket sock;

private:
QPWRDClient * const q_ptr;
Q_DECLARE_PUBLIC(QPWRDClient);
};

QPWRDClient::QPWRDClient(QObject *parent):QObject(parent),d_ptr(new QPWRDClientPrivate(this))
{
}


QPWRDClient::~QPWRDClient()
{

}

bool QPWRDClient::connect(QString pipe)
{
    Q_D(QPWRDClient);
    if (d->sock.state() == QLocalSocket::ConnectedState)
    {
        d->sock.disconnect();
        d->sock.waitForDisconnected();
        //if (stream)
        //    delete stream;
    }
    d->sock.connectToServer(pipe);
    if (!d->sock.waitForConnected())
    {
        return false;
    }
    return true;
}

void QPWRDClient::disconnect()
{
    Q_D(QPWRDClient);
    if (d->sock.state() == QLocalSocket::ConnectedState)
    {
        d->sock.disconnect();
        d->sock.waitForDisconnected();
    }
}

bool QPWRDClient::getHardwareInfo(PWRDHardwareInfo& out)
{
    Q_D(QPWRDClient);

    QJsonObject request;
    QTextStream stream(&d->sock);
    request[MSGTYPE_COMMAND] = QString (COMMAND_HWINFO);

    stream<<QJsonObjectToMessage(request);
    stream.flush();

    if (!d->sock.waitForReadyRead())
        return false;

    QString respstr = stream.readLine();
    QJsonDocument resp = QJsonDocument::fromJson(respstr.toUtf8());
    QJsonObject root = resp.object();

    JSONHWInfo basic;

    QVector<JSONBatteryHardware> batteries;
    QVector<JSONBacklightHardware> backlights;

    if (root.find(JSONHWInfo().myname()) != root.end())
    {
        basic.fromJSON ( root[basic.myname()].toObject() );
        out.basic = basic;
    }

    if (root.find(JSONBatteryHardware().myname())!= root.end())
    {
        QJsonArray arr = root[JSONBatteryHardware().myname()].toArray();
        for(int i=0; i<arr.size(); i++)
        {
            JSONBatteryHardware entry;
            entry.fromJSON(arr[i].toObject());
            out.batteries.push_back(entry);
        }
    }

    if (root.find(JSONBacklightHardware().myname())!= root.end())
    {
        QJsonArray arr = root[JSONBacklightHardware().myname()].toArray();
        for(int i=0; i<arr.size(); i++)
        {
            JSONBacklightHardware entry;
            entry.fromJSON(arr[i].toObject());
            out.backlights.push_back(entry);
        }
    }

    return true;
}

int QPWRDClient::getBacklightLevel(int backlight)
{
    Q_D(QPWRDClient);

    QJsonObject request;
    QTextStream stream(&d->sock);
    request[MSGTYPE_COMMAND] = QString (COMMAND_GET_BACKLIGHT);
    //request[BACKLIGHT_NUMBER] = QString::number(backlight);

    stream<<QJsonObjectToMessage(request);
    stream.flush();

    if (!d->sock.waitForReadyRead())
        return false;

    QString respstr = stream.readLine();
    QJsonDocument resp = QJsonDocument::fromJson(respstr.toUtf8());
    QJsonObject root = resp.object();

    return 0;
}

bool QPWRDClient::setBacklightLevel(int level, int backlight)
{
    Q_D(QPWRDClient);

    QJsonObject request;
    QTextStream stream(&d->sock);
    request[MSGTYPE_COMMAND] = QString (COMMAND_SET_BACKLIGHT);
    request[BACKLIGHT_NUMBER] = QString::number(backlight);

    stream<<QJsonObjectToMessage(request);
    stream.flush();

    if (!d->sock.waitForReadyRead())
        return false;

    QString respstr = stream.readLine();
    QJsonDocument resp = QJsonDocument::fromJson(respstr.toUtf8());
    QJsonObject root = resp.object();

    return true;
}

void QPWRDClient::pwrdRead()
{

}

