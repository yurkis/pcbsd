#include "pwrserver.h"

#include <QCoreApplication>
#include <QFile>
#include <QDebug>
#include <QTimer>
#include <QTextStream>

#include <signal.h>

//const char* const PIPE_NAME = "/var/run/pc-pwrd.pipe";
const char* const PIPE_NAME = "/home/yurkis/pc-pwrd.pipe";

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

bool PwrServer::start()
{
    if( !QLocalServer::removeServer(PIPE_NAME) )
    {
        qDebug() << "A previous instance of the pc-pwrd server is still running! Exiting...";
        exit(1);
    }
    if( server->listen(PIPE_NAME) )
    {
        QFile::setPermissions("/var/run/syscache.pipe",
                              QFile::ReadUser | QFile::WriteUser
                            | QFile::ReadGroup | QFile::WriteGroup
                            | QFile::ReadOther | QFile::WriteOther);

        qDebug() << "pc-pwrd now listening for connections at "<<PIPE_NAME;
    }
    else
    {
        qDebug() << "Error: pc-pwrd could not create pipe at "<<PIPE_NAME;
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
    QLocalServer::removeServer(PIPE_NAME); //clean up

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

