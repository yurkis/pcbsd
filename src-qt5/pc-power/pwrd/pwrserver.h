#ifndef PWRSERVER_H
#define PWRSERVER_H

#include <QObject>
#include <QLocalSocket>
#include <QLocalServer>
#include <QMap>
#include <QTextStream>

class PwrServer : public QObject{
    Q_OBJECT
public:
    PwrServer(QObject *parent=0);
    ~PwrServer();



private:

    typedef struct _SConnection
    {
        QLocalSocket* sock;
        QTextStream*  stream;
    }SConnection;

    QLocalServer *server;
    QLocalSocket *curSock;
    QMap<QLocalSocket*, SConnection> connections;

signals:

public slots:
    bool start();
    void stop();

    void signalHandler(int sig);


private slots:
    void onNewConnection();
    void onRequest();
    void onDisconnect();
};

#endif // PWRSERVER_H
