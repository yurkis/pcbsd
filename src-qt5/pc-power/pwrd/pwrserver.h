#ifndef PWRSERVER_H
#define PWRSERVER_H

#include <QObject>
#include <QLocalSocket>
#include <QLocalServer>
#include <QMap>
#include <QTextStream>
#include <QVector>

#include "pwrd.h"
#include "settingsreader.h"
#include "profilereader.h"

class PwrServer : public QObject{
    Q_OBJECT
public:
    PwrServer(QObject *parent=0);
    ~PwrServer();

private:

    PWRServerSettings settings;

    typedef struct _SConnection
    {
        QLocalSocket* sock;
        QTextStream*  stream;
    }SConnection;

    QLocalServer *server;
    QLocalSocket *curSock;
    QMap<QLocalSocket*, SConnection> connections;

    PWRBatteryHardware   battHW;
    PWRBacklightHardware backlightHW;
    PWRACPIInfo          ACPIInfo;

    PWRSuppllyInfo       current;

    QVector<PWRProfileReader>  profiles;
    PWRProfileReader           currProfile;

    void readSettings(QString confFile = QString());

signals:

public slots:
    bool start(QStringList args = QStringList());
    void stop();

    void signalHandler(int sig);


private slots:
    void onNewConnection();
    void onRequest();
    void onDisconnect();

};

#endif // PWRSERVER_H
