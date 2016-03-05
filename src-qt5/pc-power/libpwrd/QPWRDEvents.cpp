#include "QPWRDEvents.h"

#include"protocol.h"
#include "serialize.h"

#include <QLocalSocket>
#include <QJsonObject>
#include <QJsonDocument>

class QPWRDEventsPrivate:public QObject
{
    Q_OBJECT
public:
    QPWRDEventsPrivate(QPWRDEvents *parent)
        : QObject(parent), q_ptr(parent){}

    QLocalSocket sock;
public:
    bool pwrd_connect(QString pipe);
    void pwrd_disconnect();

private slots:
    void onEvent();

private:
    QPWRDEvents * const q_ptr;
    Q_DECLARE_PUBLIC(QPWRDEvents)
};

QPWRDEvents::QPWRDEvents(QObject *parent):QObject(parent),d_ptr(new QPWRDEventsPrivate(this))
{

}

QPWRDEvents::~QPWRDEvents()
{

}

bool QPWRDEvents::connect(QString pipe)
{
    //connect(&devdSocket, SIGNAL(readyRead()), this, SLOT(onDEVDEvent()));
    Q_D(QPWRDEvents);
    return d->pwrd_connect(pipe);
}

void QPWRDEvents::disconnect()
{
    Q_D(QPWRDEvents);
    d->pwrd_disconnect();
}

bool QPWRDEventsPrivate::pwrd_connect(QString pipe)
{
    if (sock.state() == QLocalSocket::ConnectedState)
    {
        sock.disconnect();
//        sock.waitForDisconnected();
        //if (stream)
        //    delete stream;
    }
    sock.connectToServer(pipe);
    if (!sock.waitForConnected())
    {
        return false;
    }
    connect(&sock, SIGNAL(readyRead()), this, SLOT(onEvent()));
    qDebug()<<"Connected!";

    return true;
}

void QPWRDEventsPrivate::pwrd_disconnect()
{

}

#define GET_VAL(var,val_name)\
    if ( !event.contains(val_name) ) return;\
    var=event[val_name]

void QPWRDEventsPrivate::onEvent()
{
    Q_Q(QPWRDEvents);

    QTextStream stream(&sock);
    while(!stream.atEnd())
    {
        QString line;
        line = stream.readLine();
        //qDebug()<<line;
        QJsonDocument evdoc = QJsonDocument::fromJson(line.toUtf8());
        QJsonObject event = evdoc.object();

        if (event.find(EVENT_EVENT_FIELD) == event.end())
            return;

        if (event[EVENT_EVENT_FIELD] == EVENT_BACKLIGHT_CHANGED)
        {
            int no, val;
            GET_VAL(no, BACKLIGHT_NUMBER).toInt();
            GET_VAL(val, BACKLIGHT_VALUE).toInt();
            emit(q->backlightChanged(no, val));
        }
        else if(event[EVENT_EVENT_FIELD] == EVENT_BATT_CAPACITY_CHANGED)
        {
            int no;
            JSONBatteryStatus stat;
            GET_VAL(no, BATTERY_NO).toInt();
            if (!event.contains(JSONBatteryStatus().myname())) return;
            if (!stat.fromJSON(event[JSONBatteryStatus().myname()].toObject())) return;
            emit(q->batteryCapacityChanged(no, stat));
        }
        else if(event[EVENT_EVENT_FIELD] == EVENT_BATT_STATE_CHANGED)
        {
            int no;
            JSONBatteryStatus stat;
            GET_VAL(no, BATTERY_NO).toInt();
            if (!event.contains(JSONBatteryStatus().myname())) return;
            if (!stat.fromJSON(event[JSONBatteryStatus().myname()].toObject())) return;
            emit(q->batteryStateChanged(no, stat));
        }
        else if (event[EVENT_EVENT_FIELD] == EVENT_AC_POWER_CHANGED)
        {
            bool isAC;
            GET_VAL(isAC, AC_POWER).toBool();
            emit(q->acLineStateChanged(isAC));
        }
        else if (event[EVENT_EVENT_FIELD] == EVENT_PROFILE_CHANGED)
        {
            QString profileID;
            GET_VAL(profileID, PROFILE_ID).toString();
            emit(q->profileChanged(profileID));
        }
        else if (event[EVENT_EVENT_FIELD] == EVENT_BUTTONS_STATE_CHANGED)
        {
            QString sleep,power,lid;
            GET_VAL(sleep, BTN_SLEEP_STATE).toString();
            GET_VAL(power, BTN_POWER_STATE).toString();
            GET_VAL(lid, LID_SWITCH_SATE).toString();
            emit(q->buttonsStateChanged(power,sleep,lid));
        }

    }
}

#include "QPWRDEvents.moc"
