/**************************************************************************
*   Copyright (C) 2015 by Yuri Momotyuk                                   *
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

#include "pwrserver.h"
#include "hw/battery.h"
#include "hw/backlight.h"
#include "hw/buttons.h"
#include "hw/intel_backlight.h"
#include "sysctlutils.h"
#include "serialize.h"

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

///////////////////////////////////////////////////////////////////////////////
PwrServer::PwrServer(QObject *parent): QObject(parent)
{
    server = new QLocalServer(this);
    connect(server, SIGNAL(newConnection()), this, SLOT(onNewConnection()));    
}

///////////////////////////////////////////////////////////////////////////////
PwrServer::~PwrServer()
{
    stop();
}

///////////////////////////////////////////////////////////////////////////////
void PwrServer::checkHardware()
{
    int i=0;

    JSONBatteryHardware    batthw;
    JSONBacklightHardware  backlighthw;

    battHW.clear();
    backlightHW.clear();

    // Ugly code for getting number of abtteries, I know
    while(getBatteryHWInfo(i++, batthw))
    {
        battHW.push_back(batthw);
    }
    hwInfo.numBatteries = battHW.size();

    // Ugly code for getting number of backlights, yes I know
    i=0;
    while(getBacklightHWInfo(i++, backlighthw))
    {
        backlightHW.push_back(backlighthw);
    }
    hwInfo.numBacklights = backlightHW.size();

    // If using intel_backlight port and intel_backlight exist
    settings.usingIntel_backlight &= hasIntelBacklight();

    hwInfo.hasSleepButton = sysctlPresent(SLEEP_BUTTON_SYSCTL);
    hwInfo.hasLid = sysctlPresent(LID_SYSCTL);
    hwInfo.possibleACPIStates = sysctl(POSSIBLE_STATES_SYSCTL).split(" ");

}

///////////////////////////////////////////////////////////////////////////////
void PwrServer::readSettings(QString confFile)
{
    checkHardware();
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
PWRProfileReader PwrServer::findProfile(QString id)
{
    PWRProfileReader ret = PWRProfileReader();
    if (profiles.contains(id))
        ret = profiles[id];
    return ret;
}

///////////////////////////////////////////////////////////////////////////////
void PwrServer::applyProfile(QString id)
{
    PWRProfileReader p = findProfile(id);
    qDebug()<<"Changing profile to "<<id;
    setblGlobalLevel( p.lcdBrightness);

    setSleepBtnSleepState(p.btnSleepSate);
    setPowerBtnSleepState(p.btnPowerSate);
    setLidSleepState(p.lidSwitchSate);
}

///////////////////////////////////////////////////////////////////////////////
int PwrServer::blGlobalLevel()
{
    if (settings.usingIntel_backlight)
        return IBLBacklightLevel();
    if (backlightHW.size())
        return backlightLevel(0);
    return 100;
}

///////////////////////////////////////////////////////////////////////////////
void PwrServer::setblGlobalLevel(int value)
{
    if (settings.usingIntel_backlight)
       setIBLBacklightLevel(value);
    else
        for (int i=0; i<backlightHW.size(); i++)
            setBacklightLevel(i, value);
}

///////////////////////////////////////////////////////////////////////////////
bool PwrServer::isOnACPower()
{
    return sysctlAsInt(ACLINE_SYSCTL) == 1;
}

///////////////////////////////////////////////////////////////////////////////
void PwrServer::onSuspend()
{
    qDebug()<<"Preparing to suspend...";
    if (settings.usingIntel_backlight)
    {
        savedBacklight = IBLBacklightLevel();
    }
}

///////////////////////////////////////////////////////////////////////////////
void PwrServer::onResume()
{
    qDebug()<<"Performing resume...";
    if (settings.usingIntel_backlight)
    {
        setIBLBacklightLevel(savedBacklight);
    }
    checkState();
}

///////////////////////////////////////////////////////////////////////////////
bool PwrServer::start(QStringList args)
{
    Q_UNUSED(args)

    QString confFile = DEF_CONFIG_FILE;
    for(int i=0; i<args.size(); i++)
    {
        if ((args[i] == "-c")&&(i<args.size()-1))
        {
            confFile = args[++i];
            continue;
        }
    }

    readSettings(confFile);    

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

    checkState(true);

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

    return true;
}

///////////////////////////////////////////////////////////////////////////////
void PwrServer::stop()
{
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
    QTextStream* devdStream = new QTextStream(&devdSocket);
    QStringList ev = devdStream->readLine().split(" ");
    QString sys,subsys;
    //"!system=ACPI", "subsystem=ACAD", "type=\_SB_.PCI0.AC0_", "notify=0x01"
    //qDebug()<<ev;
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
    }
}

///////////////////////////////////////////////////////////////////////////////
void PwrServer::onNewConnection()
{
    qDebug()<<"---------- New connection";

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
    qDebug()<<"---------- onDisconnect";

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
void PwrServer::onRequest()
{
    qDebug()<<"---------- onRequest";

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
    bool currPower = isOnACPower();
    if ((currPower == onACPower) && (!force))
        return;
    onACPower = currPower;

    applyProfile(onACPower?settings.onACProfile:settings.onBatteryProfile);

    qDebug()<<"state changed";
}

