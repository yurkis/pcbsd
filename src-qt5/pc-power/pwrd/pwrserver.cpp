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
#include "battery.h"
#include "backlight.h"
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

_str_constant SLEEP_BUTTON_SYSCTL = "hw.acpi.sleep_button_state";
_str_constant LID_SYSCTL = "hw.acpi.lid_switch_state";
_str_constant POSSIBLE_STATES_SYSCTL = "hw.acpi.supported_sleep_state";

///////////////////////////////////////////////////////////////////////////////
PwrServer::PwrServer(QObject *parent): QObject(parent)
{
    curSock = NULL;
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
    PWRSuppllyInfo        currbatt;

    battHW.clear();
    currState.clear();
    backlightHW.clear();

    // Ugly code for getting number of abtteries, I know
    while(getBatteryHWInfo(i++, batthw, currbatt))
    {
        battHW.push_back(batthw);
        currState.push_back(currbatt);
    }
    hwInfo.numBatteries = battHW.size();

    // Ugly code for getting number of backlights, yes I know
    i=0;
    while(getBacklightHWInfo(i++, backlighthw))
    {
        backlightHW.push_back(backlighthw);
    }
    hwInfo.numBacklights = backlightHW.size();

    hwInfo.hasSleepButton = sysctlPresent(SLEEP_BUTTON_SYSCTL);
    hwInfo.hasLid = sysctlPresent(LID_SYSCTL);
    hwInfo.possibleACPIStates = sysctl(POSSIBLE_STATES_SYSCTL).split(" ");

    qDebug()<<hwInfo.toJSONString();
    QJsonObject obj;
    QVector2JSON("batteries", battHW, obj);
    qDebug()<<QJsonObject2String(obj);

    qDebug()<<QJsonObject2String(QVector2JSON("backlights", backlightHW));

}

///////////////////////////////////////////////////////////////////////////////
void PwrServer::readSettings(QString confFile)
{
    checkHardware();

    settings.load(confFile);

    profiles.clear();

    //read all profiles
    QString path = settings.profilesPath;
    QDir dir(path);
    if (!dir.exists(path))
    {
        //set single default profile
        profiles.push_back(PWRProfileReader());
        currProfile = PWRProfileReader();
        return;
    }
    QStringList dir_list =dir.entryList(QStringList("*.profile"));

    for (int i=0; i<dir_list.size(); i++)
    {
        PWRProfileReader item;
        if (item.read(dir.absoluteFilePath(dir_list[i])))
        {
            profiles.push_back(item);
        }
    }
    if (!profiles.size())
    {
        profiles.push_back(PWRProfileReader());
        currProfile = PWRProfileReader();
    }
}

///////////////////////////////////////////////////////////////////////////////
void PwrServer::checkState()
{

}

///////////////////////////////////////////////////////////////////////////////
void PwrServer::sendResponse(QJsonObject resp, QTextStream *stream)
{
    QString jsontext = QJsonObjectToMessage(resp);
    qDebug()<<jsontext;
    (*stream)<<jsontext;
    stream->flush();
}

///////////////////////////////////////////////////////////////////////////////
void PwrServer::oncmdGetHWInfo(QTextStream *stream)
{
    QJsonObject resp;

    resp[hwInfo.myname()] = hwInfo.toJSON();
    QVector2JSON(JSONBatteryHardware().myname(), battHW, resp);
    QVector2JSON(JSONBacklightHardware().myname(), backlightHW, resp);

    sendResponse(resp, stream);
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

        QJsonDocument jsonResponse = QJsonDocument::fromJson(line.toUtf8());
        QJsonObject root = jsonResponse.object();

        if (root.find(MSGTYPE_COMMAND) != root.end())
        {
            qDebug()<<"COMMAND";
            if (root[MSGTYPE_COMMAND] == COMMAND_HWINFO)
            {
                oncmdGetHWInfo(connections[sender].stream);
            }
        }

        qDebug()<<line;

    }
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

    delete connections[sender].sock;
    delete connections[sender].stream;

    connections.remove(sender);

}

///////////////////////////////////////////////////////////////////////////////
void PwrServer::onStateChanged()
{

}

