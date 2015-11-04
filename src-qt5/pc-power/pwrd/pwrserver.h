#ifndef PWRSERVER_H
#define PWRSERVER_H

#include <QObject>
#include <QLocalSocket>
#include <QLocalServer>
#include <QMap>
#include <QTextStream>
#include <QVector>

#include "pwrdtypes.h"
#include "settingsreader.h"
#include "profilereader.h"
#include "serialize.h"

class PwrServer : public QObject{
    Q_OBJECT
public:
    PwrServer(QObject *parent=0);
    ~PwrServer();

private:
    //! pwrd settings
    PWRServerSettings settings;

    //! Structure holds pwrd pipe client connection
    typedef struct _SConnection
    {
        QLocalSocket* sock;    ///< Local socket
        QTextStream*  stream;  ///< Text stream associated with sock
    }SConnection;

    //! Local pipe server object
    QLocalServer *server;
    //! Client connections
    QMap<QLocalSocket*, SConnection> connections;
    //! devd pipe connection socket
    QLocalSocket devdSocket;
    //! text stream associated with devdSocket
    QTextStream* devdStream;

    //! Device hardware info
    JSONHWInfo                     hwInfo;
    //! Hardware info for all installet batteries
    QVector<JSONBatteryHardware>   battHW;
    //! Hardware info for all found backlights
    QVector<JSONBacklightHardware> backlightHW;

    QVector<PWRSuppllyInfo>       currState;

    //! True if system is on AC power
    bool onACPower;

    //! Power profiles (key is profile id)
    QMap<QString, PWRProfileReader>  profiles;

    PWRProfileReader         currProfile;

    int savedBacklight;

    //! Get all hadware info
    void checkHardware();
    //! Read daemon configuration
    void readSettings(QString confFile = QString());

    QJsonObject parseCommand(QString line);

    //! GetHWInfo command handler
    QJsonObject oncmdGetHWInfo();
    QJsonObject oncmdGetBacklight();
    QJsonObject oncmdSetBacklight(QJsonObject req);
    QJsonObject oncmdGetActiveProfiles();
    QJsonObject oncmdGetProfiles();
    QJsonObject oncmdGetProfile(QJsonObject req);


    PWRProfileReader findProfile(QString id);
    void applyProfile(QString id);

    //! Get backlight level (if more than one get lcd0 level)
    int blGlobalLevel();
    //! Set backlight level (for all backlights if more than one)
    void setblGlobalLevel(int value);

    void onSuspend();
    void onResume();

signals:

public slots:
    bool start(QStringList args = QStringList());
    void stop();

    //! UNIX signal handler
    void signalHandler(int sig);
    //! Reads event from devd when present
    void onDEVDEvent();


private slots:
    //! New client connection handler
    void onNewConnection();
    //! Client request handler
    void onRequest();
    //! Client disconnect handler
    void onDisconnect();
    //! Check power state and apply new profile when state changed
    void checkState(bool force=false);

};

#endif // PWRSERVER_H
