/**************************************************************************
*   Copyright (C) 2015- by Yuri Momotyuk                                   *
*   yurkis@pcbsd.org                                                      *
*                                                                         *
*   Permission is hereby granted, free of charge, to any person obtaining *
*   a copy of this software and associated documentation files (the       *
*   "Software"), to deal in the Software without restriction, including   *
*   without limitation the rights to use, copy, modify, merge, publish,   *
*   distribute, sublicense, and/or sell copies of the Software, and to    *
*   permit persons to whom the Software is furnished to do so, subject to *
*   the following conditions:                                             *
*                                                                         *
*   The above copyright notice and this permission notice shall be        *
*   included in all copies or substantial portions of the Software.       *
*                                                                         *
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       *
*   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    *
*   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*
*   IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR     *
*   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, *
*   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR *
*   OTHER DEALINGS IN THE SOFTWARE.                                       *
***************************************************************************/
/*!
\file
\brief Main PWRD class implementation (except JSON command handlers)
*/

#include "pwrserver.h"
#include "hw/battery.h"
#include "hw/backlight.h"
#include "hw/buttons.h"
#include "hw/intel_backlight.h"
#include "hw/sleep.h"
#include "sysctlutils.h"
#include "serialize.h"

#include "pwrddebug.h"

#include <QCoreApplication>
#include <QFile>
#include <QDebug>
#include <QTimer>
#include <QTextStream>
#include <QDir>
#include <QJsonObject>
#include <QJsonDocument>

#include "protocol.h"
#include <signal.h>

#define _str_constant static const char* const


_str_constant DEVD_PIPE = "/var/run/devd.pipe";
_str_constant SLEEP_BUTTON_SYSCTL = "hw.acpi.sleep_button_state";
_str_constant LID_SYSCTL = "hw.acpi.lid_switch_state";
_str_constant POSSIBLE_STATES_SYSCTL = "hw.acpi.supported_sleep_state";
_str_constant ACLINE_SYSCTL = "hw.acpi.acline";

const int CHECK_INTERVAL = 2000;

///////////////////////////////////////////////////////////////////////////////
PwrServer::PwrServer(QObject *parent): QObject(parent)
{
    server = new QLocalServer(this);
    connect(server, SIGNAL(newConnection()), this, SLOT(onNewConnection()));    

    eventServer = new QLocalServer(this);
    connect(eventServer, SIGNAL(newConnection()), this, SLOT(onEventNewConnection()));

    checkStateTimer = new QTimer(this);
    QObject::connect(checkStateTimer, SIGNAL(timeout()), this, SLOT(checkState()));

    isLidClosed = false;
}

///////////////////////////////////////////////////////////////////////////////
PwrServer::~PwrServer()
{
    stop();
}

///////////////////////////////////////////////////////////////////////////////
void PwrServer::checkHardware()
{
    TRACED_FN

    int i=0;

    JSONBatteryHardware    batthw;
    JSONBacklightHardware  backlighthw;

    battHW.clear();
    backlightHW.clear();

    // Ugly code for getting number of abtteries, I know
    while(getBatteryHWInfo(i++, batthw))
    {
        battHW.push_back(batthw);
        PWRBatteryStatus currState;
        if (getBatteryStatus(i, currState))
                currBatteryStates.push_back(currState);

    }
    hwInfo.numBatteries = battHW.size();

    i=0;
    if (!settings.usingIntel_backlight)
    {
        // Ugly code for getting number of backlights, yes I know
        while(getBacklightHWInfo(i++, backlighthw))
        {
            backlightHW.push_back(backlighthw);
            currBacklightLevels.push_back(backlightLevel(i));
         }
    }
    else
    {
        //intel_backlight port
        currBacklightLevels.push_back(IBLBacklightLevel());
    }
    hwInfo.numBacklights = (!settings.usingIntel_backlight)?backlightHW.size():1;

    // If using intel_backlight port and intel_backlight exist
    if (settings.usingIntel_backlight) settings.usingIntel_backlight = hasIntelBacklight();

    hwInfo.hasSleepButton = sysctlPresent(SLEEP_BUTTON_SYSCTL);
    hwInfo.hasLid = sysctlPresent(LID_SYSCTL);
    hwInfo.possibleACPIStates = sysctl(POSSIBLE_STATES_SYSCTL).split(" ");
}

///////////////////////////////////////////////////////////////////////////////
void PwrServer::readSettings(QString confFile)
{
    TRACED_FN

    //checkHardware();
    qDebug()<<"Load settings from "<<confFile<<" ...";
    settings.load(confFile);
    if (settings.usingIntel_backlight) qDebug()<<"Using intel_backlight for LCD brightness";

    profiles.clear();

    qDebug()<<"Load profiles from "<<settings.profilesPath<<" ...";

    //Add default profile
    profiles[PWRProfileReader().id] = PWRProfileReader();
    currProfile = PWRProfileReader();

    //at first read default profiles
    QString path = settings.profilesPath + QString("/default/");
    QDir dir(path);
    if (dir.exists(path))
    {
        QStringList dir_list =dir.entryList(QStringList("*.profile"));

        for (int i=0; i<dir_list.size(); i++)
        {
            PWRProfileReader item;
            if (item.read(dir.absoluteFilePath(dir_list[i])))
            {
                profiles[item.id] = item;
                qDebug()<<"    - "<<item.id<<"(default)";
            }
        }
    }

    // ..and then read user defined profiles
    // if some profile id is equal to default profile id- override
    path = settings.profilesPath;
    dir = QDir(path);
    if (dir.exists(path))
    {
        QStringList dir_list =dir.entryList(QStringList("*.profile"));

        for (int i=0; i<dir_list.size(); i++)
        {
            PWRProfileReader item;
            if (item.read(dir.absoluteFilePath(dir_list[i])))
            {
                profiles[item.id] = item;
                qDebug()<<"    - "<<item.id;
            }
        }
    }

    // Set defaults for profiles if not set in config file
    if (!settings.onBatteryProfile.length())
    {
        settings.onBatteryProfile = (profiles.contains(DEF_ON_BATTERY_PROFILE_ID))?DEF_ON_BATTERY_PROFILE_ID:DEF_PROFILE_ID;
    }
    if (!settings.onACProfile.length())
    {
        settings.onACProfile = (profiles.contains(DEF_ON_AC_POWER_PROFILE_ID))?DEF_ON_AC_POWER_PROFILE_ID:DEF_PROFILE_ID;
    }
    if (!settings.onLowBatteryProfile.length())
    {
        settings.onLowBatteryProfile = (profiles.contains(DEF_ON_LOW_POWER_PROFILE_ID))?DEF_ON_LOW_POWER_PROFILE_ID:DEF_PROFILE_ID;
    }
}

///////////////////////////////////////////////////////////////////////////////
void PwrServer::emitEvent(QString event_name, QJsonObject event)
{
    TRACED_FN

    event[EVENT_EVENT_FIELD] = event_name;

    QString json = QJsonObjectToMessage(event) + "\n";

    for(auto it = eventConnections.begin(); it!= eventConnections.end(); ++it)
    {
        QTextStream* stream = it.value().stream;
        (*stream)<<json;
        stream->flush();
    }
}

///////////////////////////////////////////////////////////////////////////////
void PwrServer::emitBacklightChanged(int backlight, int level)
{
    TRACED_FN

    QJsonObject event;
    event[BACKLIGHT_NUMBER]= (int)backlight;
    event[BACKLIGHT_VALUE]= level;
    emitEvent(EVENT_BACKLIGHT_CHANGED, event);
}

///////////////////////////////////////////////////////////////////////////////
PWRProfileReader PwrServer::findProfile(QString id)
{
    TRACED_FN

    PWRProfileReader ret = PWRProfileReader();
    if (profiles.contains(id))
        ret = profiles[id];
    return ret;
}

///////////////////////////////////////////////////////////////////////////////
void PwrServer::applyProfile(QString id)
{
    TRACED_FN

    PWRProfileReader p = findProfile(id);
    currProfile = p;
    qDebug()<<"Changing profile to "<<id;
    setblGlobalLevel( p.lcdBrightness);
    checkBacklights();

    setSleepBtnSleepState(p.btnSleepSate);
    setPowerBtnSleepState(p.btnPowerSate);
    setLidSleepState(p.lidSwitchSate);
    checkButtons();

    QJsonObject event;
    event[PROFILE_ID] = id;
    emitEvent(EVENT_PROFILE_CHANGED, event);

    //if profile changed and lid is closed try to set sleep state if present
    //So if we close notebook after turning off external power notebook may sleep
    if (isLidClosed)
    {
        if (p.lidSwitchSate.trimmed().toUpper() != "NONE")
        {
            ACPISleep(p.lidSwitchSate);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
void PwrServer::checkBacklights()
{
    TRACED_FN

    int level=0;
    if (!settings.usingIntel_backlight)
    {
        for(int i=0; i<backlightHW.size(); i++)
        {
            qDebug()<<"2"<<currBacklightLevels.size();
            level = backlightLevel(i);
            if (level != currBacklightLevels[i])
            {
                //emit event
                //qDebug()<<"Backlight changed to "<<level;
                currBacklightLevels[i] = level;
                emitBacklightChanged(i, level);
            }
        }//for all backlights
    }
    else
    {        
        level = IBLBacklightLevel();
        if (level != currBacklightLevels[0])
        {
            //emit event            
            currBacklightLevels[0] = level;
            emitBacklightChanged(0, level);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
void PwrServer::checkBatts(bool* hasLowBattery)
{
    TRACED_FN

    if (hasLowBattery) (*hasLowBattery) = false;
    for(int i=0; i<battHW.size(); i++)
    {
        //QVector<PWRBatteryStatus> currBatteryStates;
        JSONBatteryStatus curr;
        if (getBatteryStatus(i, curr))
        {
            curr.batteryCritical = (int)curr.batteryCapacity<=settings.lowBatteryCapacity;
            if ((curr.batteryState == BATT_DISCHARGING) && ((int)curr.batteryCapacity <= settings.lowBatteryCapacity))
            {
                if (hasLowBattery) (*hasLowBattery) = true;
            }
            if (curr.batteryCapacity != currBatteryStates[i].batteryCapacity)
            {
                QJsonObject event;
                event[BATTERY_NO] = i;
                event[JSONBatteryStatus().myname()] = curr.toJSON();
                emitEvent(EVENT_BATT_CAPACITY_CHANGED, event);
            }
            if (curr.batteryState != currBatteryStates[i].batteryState)
            {
                QJsonObject event;
                event[BATTERY_NO] = i;
                event[JSONBatteryStatus().myname()] = curr.toJSON();
                emitEvent(EVENT_BATT_STATE_CHANGED, event);
            }
            if (currBatteryStates.size() > i)
                currBatteryStates[i] = curr;
        }//if got battery status
    }//for all batteries
}

///////////////////////////////////////////////////////////////////////////////
void PwrServer::checkButtons()
{
    TRACED_FN

    QString state;
    bool shouldEmitEvent=false;
    state  = sleepBtnSleepState();
    shouldEmitEvent = currSleepBtnState != state;
    currSleepBtnState = state;

    state  = powerBtnSleepState();
    shouldEmitEvent = shouldEmitEvent | (currPowerBtnState != state);
    currPowerBtnState = state;

    state = lidSleepState();
    shouldEmitEvent = shouldEmitEvent | (currLidSwitchState != state);
    currLidSwitchState = state;

    if (shouldEmitEvent)
    {
        QJsonObject event;
        event[BTN_POWER_STATE]= currPowerBtnState;
        event[BTN_SLEEP_STATE]= currSleepBtnState;
        event[LID_SWITCH_SATE]= currLidSwitchState;
        emitEvent(EVENT_BUTTONS_STATE_CHANGED, event);
    }
}

///////////////////////////////////////////////////////////////////////////////
int PwrServer::blGlobalLevel()
{
    TRACED_FN

    if (settings.usingIntel_backlight)
        return IBLBacklightLevel();
    if (backlightHW.size())
        return backlightLevel(0);
    return 100;
}

///////////////////////////////////////////////////////////////////////////////
void PwrServer::setblGlobalLevel(int value)
{
    TRACED_FN

    //TODO: emit events here
    if (settings.usingIntel_backlight)
       setIBLBacklightLevel(value);
    else
        for (int i=0; i<backlightHW.size(); i++)
            setBacklightLevel(i, value);
}

///////////////////////////////////////////////////////////////////////////////
#ifndef FAKE_BATT
bool PwrServer::isOnACPower()
{
    return sysctlAsInt(ACLINE_SYSCTL) == 1;
}
#endif
///////////////////////////////////////////////////////////////////////////////
void PwrServer::onSuspend()
{
    TRACED_FN

    qDebug()<<"Preparing to suspend...";
    if (settings.usingIntel_backlight)
    {
        savedBacklight = IBLBacklightLevel();
    }
}

///////////////////////////////////////////////////////////////////////////////
void PwrServer::onResume()
{
    TRACED_FN

    qDebug()<<"Performing resume...";
    if (settings.usingIntel_backlight)
    {
        setIBLBacklightLevel(savedBacklight);
    }
    isLidClosed = false;
    checkState();
}

///////////////////////////////////////////////////////////////////////////////
bool PwrServer::start(QStringList args)
{
    TRACED_FN

    Q_UNUSED(args)

    confFile = DEF_CONFIG_FILE;
    for(int i=0; i<args.size(); i++)
    {
        if ((args[i] == "-c")&&(i<args.size()-1))
        {
            confFile = args[++i];
            continue;
        }
    }

    readSettings(confFile);    
    checkHardware();

    //devd socket setup
    if (devdSocket.state() == QLocalSocket::ConnectedState)
    {
        devdSocket.disconnect();
        devdSocket.waitForDisconnected();
    }
    devdSocket.connectToServer(DEVD_PIPE);
    if (!devdSocket.waitForConnected())
    {
        qDebug()<<"Unable to connet to devd";
    }
    qDebug()<<"Connected to devd...";
    devdStream = new QTextStream(&devdSocket);
    //connect(server, SIGNAL(newConnection()), this, SLOT(onNewConnection()));
    connect(&devdSocket, SIGNAL(readyRead()), this, SLOT(onDEVDEvent()));

    onACPower = sysctlAsInt(ACLINE_SYSCTL) == 1;    

    //setup control pipe
    if( !QLocalServer::removeServer(settings.pipeName) )
    {
        qDebug() << "A previous instance of the pc-pwrd server is still running! Exiting...";
        exit(1);
    }
    if( server->listen(settings.pipeName) )
    {
        QFile::setPermissions(settings.pipeName,
                              QFile::ReadUser | QFile::WriteUser
                            | QFile::ReadGroup | QFile::WriteGroup
                            | QFile::ReadOther | QFile::WriteOther);

        qDebug() << "pc-pwrd now listening for connections at "<<settings.pipeName;
    }
    else
    {
        qDebug() << "Error: pc-pwrd could not create pipe at "<<settings.pipeName;
        return false;
    }

    //setup events pipe
    if( !QLocalServer::removeServer(settings.eventsPipeName) )
    {
        qDebug() << "A previous instance of the pc-pwrd server is still running! Exiting...";
        exit(1);
    }
    if( eventServer->listen(settings.eventsPipeName) )
    {
        QFile::setPermissions(settings.eventsPipeName,
                              QFile::ReadUser | QFile::WriteUser
                            | QFile::ReadGroup | QFile::WriteGroup
                            | QFile::ReadOther | QFile::WriteOther);

        qDebug() << "pc-pwrd notifies events at "<<settings.eventsPipeName;
    }
    else
    {
        qDebug() << "Error: pc-pwrd could not create pipe at "<<settings.eventsPipeName;
        return false;
    }

    checkState(true);

    checkStateTimer->setInterval(CHECK_INTERVAL);
    checkStateTimer->start();

    return true;
}

///////////////////////////////////////////////////////////////////////////////
void PwrServer::stop()
{
    TRACED_FN

    if(server->isListening())
    {
        server->close();
    }
    QLocalServer::removeServer(settings.pipeName); //clean up

    QCoreApplication::exit(0);
}

///////////////////////////////////////////////////////////////////////////////
void PwrServer::signalHandler(int sig)
{
    TRACED_FN

    switch(sig) {
        case SIGHUP:

            break;
        case SIGTERM:
            QTimer::singleShot(0, this, SLOT(stop()));
            break;
    }//switch
}

///////////////////////////////////////////////////////////////////////////////
void PwrServer::onDEVDEvent()
{
    TRACED_FN

    QTextStream* devdStream = new QTextStream(&devdSocket);

    while(!devdStream->atEnd())
    {
        QStringList ev = devdStream->readLine().split(" ");
        //"!system=ACPI", "subsystem=ACAD", "type=\_SB_.PCI0.AC0_", "notify=0x01"

        if (ev.size()<3)
            return;
        if (ev[0].replace("!system=", "") != "ACPI")
            return;
        if (ev[1].replace("subsystem=","") == "ACAD")
        {
            QTimer::singleShot(0, this, SLOT(checkState()));
            return;
        }
        else if (ev[1].replace("subsystem=","") == "Suspend")
        {
            onSuspend();
        }
        else if (ev[1].replace("subsystem=","") == "Resume")
        {
            onResume();
        }else if (ev[1].replace("subsystem=","") == "Lid")
        {
            isLidClosed = (ev[3].trimmed() == "notify=0x00")?true:false;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
void PwrServer::onNewConnection()
{
    TRACED_FN

    qDebug()<<"New control pipe connection";

    SConnection conn;
    conn.sock = server->nextPendingConnection();
    if (conn.sock)
    {
        if (!conn.sock->isValid())
            return;
    }
    else
    {
        return;
    }

    connect(conn.sock, SIGNAL(readyRead()), this, SLOT(onRequest()) );
    connect(conn.sock, SIGNAL(disconnected()), this, SLOT(onDisconnect()) );

    conn.stream = new QTextStream(conn.sock);
    connections[conn.sock]= conn;
}

///////////////////////////////////////////////////////////////////////////////
void PwrServer::onDisconnect()
{
    TRACED_FN

    qDebug()<<"Disconnect from control pipe";

    QLocalSocket* sender = (QLocalSocket*)QObject::sender();
    if (!sender)
    {
        qDebug()<<"Unknown signal sender";
        return;
    }
    if (!connections.contains(sender))
    {
        qDebug()<<"Unknown connection";
        return;
    }

    if (connections.contains(sender))
    {
        delete connections[sender].stream;
        connections.remove(sender);
    }
}

///////////////////////////////////////////////////////////////////////////////
void PwrServer::onEventNewConnection()
{
    TRACED_FN

    qDebug()<<"New event listener connection";

    SConnection conn;
    conn.sock = eventServer->nextPendingConnection();
    if (conn.sock)
    {
        if (!conn.sock->isValid())
            return;
    }
    else
    {
        return;
    }

    conn.stream = new QTextStream(conn.sock);
    eventConnections[conn.sock]= conn;

    connect(conn.sock, SIGNAL(disconnected()), this, SLOT(onEventDisconnect()) );
}

///////////////////////////////////////////////////////////////////////////////
void PwrServer::onEventDisconnect()
{
    TRACED_FN

    qDebug()<<"Event listener disconnect";

    QLocalSocket* sender = (QLocalSocket*)QObject::sender();
    if (!sender)
    {
        qDebug()<<"Unknown signal sender";
        return;
    }
    if (!eventConnections.contains(sender))
    {
        qDebug()<<"Unknown connection";
        return;
    }

    if (eventConnections.contains(sender))
    {
        delete eventConnections[sender].stream;
        eventConnections.remove(sender);
    }
}

///////////////////////////////////////////////////////////////////////////////
void PwrServer::onRequest()
{
    TRACED_FN

    //qDebug()<<"---------- onRequest";

    QLocalSocket* sender = (QLocalSocket*)QObject::sender();
    if (!sender)
    {
        qDebug()<<"Unknown signal sender";
        return;
    }
    if (!connections.contains(sender))
    {
        qDebug()<<"Unknown connection";
        return;
    }
    while(!connections[sender].stream->atEnd())
    {
        QString line;
        line = connections[sender].stream->readLine();

        QJsonObject resp= parseCommand(line);

        QString jsontext = QJsonObjectToMessage(resp);
        qDebug()<<jsontext;
        (*connections[sender].stream)<<jsontext<<"\n";
        connections[sender].stream->flush();
    }
}

///////////////////////////////////////////////////////////////////////////////
void PwrServer::checkState(bool force)
{    
    TRACED_FN

    bool currLowBatt;
    static bool wasLowBatt = false;
    bool isProfileChanges = false;
    QString profileName;

    checkBacklights();
    checkBatts(&currLowBatt);

    //qDebug()<<"Low batt: "<<currLowBatt;

    bool currPower = isOnACPower();
    
    if (force)
    {
        if (currPower)
        {
            isProfileChanges = true;
            profileName = settings.onACProfile;
        }
        else
        {
            if (!currLowBatt)
            {
                isProfileChanges = true;
                profileName = settings.onBatteryProfile;
            }
            else
            {
                wasLowBatt = currLowBatt;
                isProfileChanges = true;
                profileName = settings.onLowBatteryProfile;
            }
        }
    }
    else
    {
        if (currPower)
        {
            if (!onACPower)
            {
                isProfileChanges = true;
                profileName = settings.onACProfile;
            }
            wasLowBatt = false;
        }
        else
        {
            //on battery
            if ( (currLowBatt) && (!wasLowBatt) )
            {
                //low battery
                wasLowBatt = currLowBatt;
                isProfileChanges = true;
                profileName = settings.onLowBatteryProfile;
            }
            else if (onACPower)
            {
                //battery
                isProfileChanges = true;
                profileName = settings.onBatteryProfile;
            }
        }
    }

    if (currPower != onACPower)
    {
        QJsonObject event;
        event[AC_POWER]= currPower;
        emitEvent(EVENT_AC_POWER_CHANGED, event);
    }

    if (isProfileChanges)
    {
        qDebug()<<"Profile changed to "<<profileName;
        applyProfile(profileName);
    }
    onACPower = currPower;
}

