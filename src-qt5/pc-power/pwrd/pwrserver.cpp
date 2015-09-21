#include "pwrserver.h"
#include "battery.h"

#include <QCoreApplication>
#include <QFile>
#include <QDebug>
#include <QTimer>
#include <QTextStream>
#include <QDir>

#include <signal.h>

#include "backlight.h"

PwrServer::PwrServer(QObject *parent): QObject(parent)
{
    curSock = NULL;
    server = new QLocalServer(this);
    connect(server, SIGNAL(newConnection()), this, SLOT(onNewConnection()));
}

PwrServer::~PwrServer()
{
    stop();
}

void PwrServer::readSettings(QString confFile)
{
    settings.load(confFile);

    profiles.clear();

    //read all profiles
    QString path = settings.profilesPath;
    qDebug()<<settings.profilesPath;
    QDir dir(path);
    if (!dir.exists(path))
    {
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
        qDebug()<<item.name;
    }
    if (!profiles.size())
    {
        profiles.push_back(PWRProfileReader());
        currProfile = PWRProfileReader();
    }
    qDebug()<<profiles.size();
}

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
    qDebug()<<confFile;

    readSettings(confFile);

    getBatteryHWInfo(0, battHW, current);
    getBacklightHWInfo(backlightHW);

    if( !QLocalServer::removeServer(settings.pipeName) )
    {
        qDebug() << "A previous instance of the pc-pwrd server is still running! Exiting...";
        exit(1);
    }
    if( server->listen(settings.pipeName) )
    {
        QFile::setPermissions("/var/run/syscache.pipe",
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

void PwrServer::stop()
{
    if(server->isListening())
    {
        server->close();
    }
    QLocalServer::removeServer(settings.pipeName); //clean up

    QCoreApplication::exit(0);
}

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
        qDebug()<<line;
    }
}

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

