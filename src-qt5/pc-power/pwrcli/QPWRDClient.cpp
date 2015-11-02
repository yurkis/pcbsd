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
    QString lastError;

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

QString QPWRDClient::lastPWRDError()
{
    Q_D(QPWRDClient);
    return d->lastError;
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
    return setBacklightLevel(QString::number(level), backlight);
}

bool QPWRDClient::setBacklightLevelRelative(int level, int backlight)
{
    QString sign = (level<0)?QString("-"):QString("+");
    return setBacklightLevel(sign + QString::number(level), backlight);
}

bool QPWRDClient::setBacklightLevel(QString level, int backlight)
{
    Q_D(QPWRDClient);

    QJsonObject request;
    QTextStream stream(&d->sock);
    request[MSGTYPE_COMMAND] = QString (COMMAND_SET_BACKLIGHT);
    request[BACKLIGHT_NUMBER] = QString::number(backlight);
    request[BACKLIGHT_VALUE] = level;

    qDebug()<<QJsonObjectToMessage(request);
    stream<<QJsonObjectToMessage(request);
    stream.flush();

    if (!d->sock.waitForReadyRead())
        return false;

    QString respstr = stream.readLine();
    QJsonDocument resp = QJsonDocument::fromJson(respstr.toUtf8());
    QJsonObject root = resp.object();

    return true;
}

bool QPWRDClient::getActiveProfiles(PWRProfileInfoBasic *ac_profile, PWRProfileInfoBasic *batt_profile, PWRProfileInfoBasic *low_batt_profile)
{
    Q_D(QPWRDClient);

    d->lastError = "";
    QJsonObject request;
    QTextStream stream(&d->sock);
    request[MSGTYPE_COMMAND] = QString (COMMAND_ACTIVE_PROFILES);
    stream<<QJsonObjectToMessage(request);
    stream.flush();

    if (!d->sock.waitForReadyRead())
        return false;

    QString respstr = stream.readLine();
    QJsonDocument resp = QJsonDocument::fromJson(respstr.toUtf8());
    QJsonObject root = resp.object();

    if (!root.contains(MSG_RESULT)) return false;
    if (root[MSG_RESULT].toString() != QString(MSG_RESULT_SUCCESS))
    {
        if (root.contains(MSG_RESULT_FAIL_REASON)) d->lastError = root[MSG_RESULT_FAIL_REASON].toString();
        return false;
    }

    if (ac_profile && root.contains(ON_AC_POWER_PROFILE_ID) && root.contains(ON_AC_POWER_PROFILE_NAME))
    {
        ac_profile->id = root[ON_AC_POWER_PROFILE_ID].toString();
        ac_profile->name = root[ON_AC_POWER_PROFILE_NAME].toString();
    }
    if (batt_profile && root.contains(ON_BATTERY_PROFILE_ID) && root.contains(ON_BATTERY_PROFILE_NAME))
    {
        batt_profile->id = root[ON_BATTERY_PROFILE_ID].toString();
        batt_profile->name = root[ON_BATTERY_PROFILE_NAME].toString();
    }
    if (low_batt_profile && root.contains(ON_LOW_BATTERY_PROFILE_ID) && root.contains(ON_LOW_BATTERY_PROFILE_NAME))
    {
        low_batt_profile->id = root[ON_LOW_BATTERY_PROFILE_ID].toString();
        low_batt_profile->name = root[ON_LOW_BATTERY_PROFILE_NAME].toString();
    }
    return true;
}

void QPWRDClient::pwrdRead()
{

}

