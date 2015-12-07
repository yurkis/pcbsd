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
    bool sendCommandReadResponce(QJsonObject request, QJsonObject& responce);

private:
QPWRDClient * const q_ptr;
Q_DECLARE_PUBLIC(QPWRDClient);
};

///////////////////////////////////////////////////////////////////////////////
bool QPWRDClientPrivate::sendCommandReadResponce(QJsonObject request, QJsonObject &responce)
{
    Q_Q(QPWRDClient);

    lastError = "";
    if (sock.state() != QLocalSocket::ConnectedState)
    {
        lastError = "Not conected to pwrd";
        emit(q->connectionError());
        return false;
    }
    QTextStream stream(&sock);
    stream<<QJsonObjectToMessage(request);
    stream.flush();
    if (!sock.waitForReadyRead())
        return false;
    QString respstr = stream.readLine();
    QJsonDocument resp = QJsonDocument::fromJson(respstr.toUtf8());
    responce = resp.object();
    if (!responce.contains(MSG_RESULT)) return false;
    if (responce[MSG_RESULT].toString() != QString(MSG_RESULT_SUCCESS))
    {
        if (responce.contains(MSG_RESULT_FAIL_REASON)) lastError = responce[MSG_RESULT_FAIL_REASON].toString();
        emit(q->pwrdError(lastError));
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
QPWRDClient::QPWRDClient(QObject *parent):QObject(parent),d_ptr(new QPWRDClientPrivate(this))
{
}

///////////////////////////////////////////////////////////////////////////////
QPWRDClient::~QPWRDClient()
{

}

///////////////////////////////////////////////////////////////////////////////
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

///////////////////////////////////////////////////////////////////////////////
void QPWRDClient::disconnect()
{
    Q_D(QPWRDClient);
    if (d->sock.state() == QLocalSocket::ConnectedState)
    {
        d->sock.disconnect();
        d->sock.waitForDisconnected();
    }
}

///////////////////////////////////////////////////////////////////////////////
QString QPWRDClient::lastPWRDError()
{
    Q_D(QPWRDClient);
    return d->lastError;
}

///////////////////////////////////////////////////////////////////////////////
bool QPWRDClient::getHardwareInfo(PWRDHardwareInfo& out)
{
    Q_D(QPWRDClient);

    QJsonObject request, root;

    d->lastError = "";

    request[MSGTYPE_COMMAND] = QString (COMMAND_HWINFO);

    if (!d->sendCommandReadResponce(request, root)) return false;

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

///////////////////////////////////////////////////////////////////////////////
bool QPWRDClient::getAllBacklighsLevel(QVector<int>& out)
{
    Q_D(QPWRDClient);

    d->lastError = "";

    QJsonObject request, root;

    out.clear();

    request[MSGTYPE_COMMAND] = QString (COMMAND_GET_BACKLIGHT);

    if (!d->sendCommandReadResponce(request, root)) return false;

    if (!root.contains(BACKLIGHT_LEVELS)) return false;
    if (!root[BACKLIGHT_LEVELS].isArray()) return false;
    QJsonArray arr = root[BACKLIGHT_LEVELS].toArray();
    for (int i=0;i<arr.size(); i++)
    {
        out.push_back(arr[i].toInt());
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
bool QPWRDClient::getBacklightLevel(int backlight, int &out)
{
    QVector<int> levels;
    if (!getAllBacklighsLevel(levels)) return false;

    if (backlight >= levels.size()) return false;

    out = levels[backlight];
    return true;
}

///////////////////////////////////////////////////////////////////////////////
bool QPWRDClient::setBacklightLevel(int level, int backlight)
{
    return setBacklightLevel(QString::number(level), backlight);
}

///////////////////////////////////////////////////////////////////////////////
bool QPWRDClient::setBacklightLevelRelative(int level, int backlight)
{
    QString sign = (level<0)?QString("-"):QString("+");
    return setBacklightLevel(sign + QString::number(level), backlight);
}

///////////////////////////////////////////////////////////////////////////////
bool QPWRDClient::setBacklightLevel(QString level, int backlight)
{
    Q_D(QPWRDClient);

    QJsonObject request, root;

    request[MSGTYPE_COMMAND] = QString (COMMAND_SET_BACKLIGHT);
    request[BACKLIGHT_NUMBER] = QString::number(backlight);
    request[BACKLIGHT_VALUE] = level;

    return d->sendCommandReadResponce(request, root);

}

///////////////////////////////////////////////////////////////////////////////
bool QPWRDClient::getActiveProfiles(PWRProfileInfoBasic *ac_profile, PWRProfileInfoBasic *batt_profile, PWRProfileInfoBasic *low_batt_profile)
{
    Q_D(QPWRDClient);

    d->lastError = "";
    QJsonObject request;
    QJsonObject root;

    request[MSGTYPE_COMMAND] = QString (COMMAND_ACTIVE_PROFILES);

    if (!d->sendCommandReadResponce(request, root)) return false;

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

///////////////////////////////////////////////////////////////////////////////
bool QPWRDClient::getProfiles(QVector<PWRProfileInfoBasic> &profiles)
{
    Q_D(QPWRDClient);

    d->lastError = "";
    profiles.clear();

    QJsonObject req, resp;

    req[MSGTYPE_COMMAND] = COMMAND_GET_PROFILES;
    if (!d->sendCommandReadResponce(req, resp)) return false;

    if (!resp.contains(PROFILES_ARRAY)) return false;
    if (!resp[PROFILES_ARRAY].isArray()) return false;
    QJsonArray arr = resp[PROFILES_ARRAY].toArray();
    for (int i=0; i<arr.size(); i++)
    {
        PWRProfileInfoBasic item;
        QJsonObject e = arr[i].toObject();
        if (!e.contains(PROFILE_ID)) continue;
        item.id = e[PROFILE_ID].toString();
        if (e.contains(PROFILE_NAME)) item.name = e[PROFILE_NAME].toString();

        profiles.push_back(item);
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
bool QPWRDClient::getProfile(QString profile_id, PWRProfile &out)
{
    Q_D(QPWRDClient);

    d->lastError = "";

    QJsonObject req, resp;

    req[MSGTYPE_COMMAND] = COMMAND_GET_PROFILE;
    if (profile_id.length())
        req[PROFILE_ID] = profile_id;

    if (!d->sendCommandReadResponce(req, resp)) return false;

    JSONProfile resp_p;
    if (!resp_p.fromJSON(resp)) return false;

    out = resp_p;

    return true;
}

///////////////////////////////////////////////////////////////////////////////
bool QPWRDClient::getCurrentProfileID(PWRProfileInfoBasic &out)
{
    Q_D(QPWRDClient);

    d->lastError = "";

    QJsonObject req, resp;
    out = PWRProfileInfoBasic();

    req[MSGTYPE_COMMAND] = COMMAND_GET_CURRENT_PROFILE;

    if (!d->sendCommandReadResponce(req, resp)) return false;

    if (resp.contains(PROFILE_ID)) out.id = resp[PROFILE_ID].toString();
    if (resp.contains(PROFILE_NAME)) out.name = resp[PROFILE_NAME].toString();

    return true;
}

bool QPWRDClient::setCurrentProfile(QString profile_id)
{
    Q_D(QPWRDClient);

    d->lastError = "";

    QJsonObject req, resp;

    req[MSGTYPE_COMMAND] = COMMAND_APPLY_PROFILE;
    req[PROFILE_ID] = profile_id;

    return d->sendCommandReadResponce(req, resp);
}

///////////////////////////////////////////////////////////////////////////////
bool QPWRDClient::getACLineState(bool& isOnACPower)
{
    Q_D(QPWRDClient);

    d->lastError = "";

    QJsonObject req, resp;
    isOnACPower = true;

    req[MSGTYPE_COMMAND] = COMMAND_AC_STATUS;

    if (!d->sendCommandReadResponce(req, resp)) return false;

    if (resp.contains(AC_POWER)) isOnACPower = resp[AC_POWER].toBool();

    return true;
}

///////////////////////////////////////////////////////////////////////////////
bool QPWRDClient::getBatteriesState(QVector<PWRBatteryStatus>& batteries)
{
    Q_D(QPWRDClient);

    d->lastError = "";

    QJsonObject req, resp;
    batteries.clear();

    req[MSGTYPE_COMMAND] = COMMAND_GET_BATT_STATE;

    if (!d->sendCommandReadResponce(req, resp)) return false;
    if (!resp.contains(JSONBatteryStatus().myname())) return false;
    if (!resp[JSONBatteryStatus().myname()].isArray()) return false;
    QJsonArray arr = resp[JSONBatteryStatus().myname()].toArray();
    for(int i=0; i<arr.size(); i++)
    {
        JSONBatteryStatus item;
        if (!item.fromJSON(arr[i].toObject())) continue;
        batteries.push_back(item);
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
bool QPWRDClient::setACPIState(QString state)
{
    Q_D(QPWRDClient);

    d->lastError = "";

    QJsonObject req, resp;

    req[MSGTYPE_COMMAND] = COMMAND_SET_ACPI_STATE;
    req[ACPI_STATE] = state;

    if (!d->sendCommandReadResponce(req, resp)) return false;

    return true;
}

///////////////////////////////////////////////////////////////////////////////
bool QPWRDClient::getButtonsState(QString &powerBtnSate, QString &sleepBtnState, QString &lidState)
{
    Q_D(QPWRDClient);

    d->lastError = "";

    QJsonObject req, resp;

    powerBtnSate = "NONE";
    sleepBtnState = "NONE";
    lidState = "NONE";

    req[MSGTYPE_COMMAND] = COMMAND_GET_BUTTONS_STATE;

    if (!d->sendCommandReadResponce(req, resp)) return false;

    if (resp.contains(BTN_POWER_STATE)) powerBtnSate = resp[BTN_POWER_STATE].toString();
    if (resp.contains(BTN_SLEEP_STATE)) sleepBtnState = resp[BTN_SLEEP_STATE].toString();
    if (resp.contains(LID_SWITCH_SATE)) lidState = resp[LID_SWITCH_SATE].toString();

    return true;
}

///////////////////////////////////////////////////////////////////////////////
bool QPWRDClient::setButtonsState(QString *powerBtnSate, QString *sleepBtnState, QString *lidState)
{
    Q_D(QPWRDClient);

    d->lastError = "";

    QJsonObject req, resp;

    if ((!powerBtnSate) && (!sleepBtnState) && (!lidState)) return true;

    req[MSGTYPE_COMMAND] = COMMAND_SET_BUTTONS_STATE;
    if (powerBtnSate) req[BTN_POWER_STATE] = *powerBtnSate;
    if (sleepBtnState) req[BTN_SLEEP_STATE] = *sleepBtnState;
    if (lidState) req[LID_SWITCH_SATE] = *lidState;

    return d->sendCommandReadResponce(req, resp);
}

///////////////////////////////////////////////////////////////////////////////
bool QPWRDClient::getDaemonSettings(PWRDaemonSettings &settings)
{
    Q_D(QPWRDClient);

    d->lastError = "";

    QJsonObject req, resp;

    req[MSGTYPE_COMMAND] = COMMAND_GET_SETTINGS;

    if (!d->sendCommandReadResponce(req, resp)) return false;

    JSONDaemonSettings jsonsettings;
    if (!jsonsettings.fromJSON(resp)) return false;

    settings = jsonsettings;

    return true;
}

///////////////////////////////////////////////////////////////////////////////
bool QPWRDClient::setDaemonSettings(PWRDaemonSettings settings)
{
    Q_D(QPWRDClient);

    d->lastError = "";

    QJsonObject req, resp;

    req[MSGTYPE_COMMAND] = COMMAND_SET_SETTINGS;
    JSONDaemonSettings s;
    s.onACProfile = settings.onACProfile;
    s.onBatteryProfile = settings.onBatteryProfile;
    s.onLowBatteryProfile = settings.onLowBatteryProfile;
    s.lowBatteryCapacity = settings.lowBatteryCapacity;
    s.toJSON(req);

    return d->sendCommandReadResponce(req, resp);
}

///////////////////////////////////////////////////////////////////////////////
bool QPWRDClient::updateProfile(PWRProfile profile)
{
    Q_D(QPWRDClient);

    d->lastError = "";

    QJsonObject req, resp;

    req[MSGTYPE_COMMAND] = COMMAND_UPDATE_PROFILE;

    JSONProfile jsonprofile = profile;
    jsonprofile.toJSON(req);

    return d->sendCommandReadResponce(req, resp);
}

///////////////////////////////////////////////////////////////////////////////
bool QPWRDClient::removeProfile(QString profile_id)
{
    Q_D(QPWRDClient);

    d->lastError = "";

    QJsonObject req, resp;

    req[MSGTYPE_COMMAND] = COMMAND_REMOVE_PROFILE;
    req[PROFILE_ID] = profile_id;

    return d->sendCommandReadResponce(req, resp);
}




