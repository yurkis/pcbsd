#ifndef LIBPWRD_H
#define LIBPWRD_H

#include <QString>
#include <QVector>

#include "pwrdtypes.h"

typedef struct PWRDHardwareInfo
{
    PWRHWInfo basic;
    QVector<PWRBatteryHardware>  batteries;
    QVector<PWRBacklightHardware>backlights;
}PWRDHardwareInfo;

typedef struct PWRProfileInfoBasic
{
    QString id;
    QString name;
}PWRProfileInfoBasic;

class QPWRDClientPrivate;

class QPWRDClient:public QObject
{    
    Q_OBJECT        
    Q_DECLARE_PRIVATE(QPWRDClient)
public:
    explicit QPWRDClient(QObject *parent=0);
    ~QPWRDClient();

    virtual bool connect(QString pipe = QString(DEF_PWRD_PIPE_NAME));
    virtual void disconnect();
    virtual QString lastPWRDError();
    virtual bool getHardwareInfo(PWRDHardwareInfo &out);
    virtual bool getAllBacklighsLevel(QVector<int>& out);
    virtual bool getBacklightLevel(int backlight, int& out);
    virtual bool setBacklightLevel(int level, int backlight = PWR_ALL);
    virtual bool setBacklightLevelRelative(int level, int backlight = PWR_ALL);
    virtual bool setBacklightLevel(QString level, int backlight = PWR_ALL);
    virtual bool getActiveProfiles(PWRProfileInfoBasic* ac_profile, PWRProfileInfoBasic* batt_profile, PWRProfileInfoBasic* low_batt_profile);
    virtual bool getProfiles(QVector<PWRProfileInfoBasic>& profiles);
    virtual bool getProfile(QString profile_id, PWRProfile& out);

signals:
    void backlightLevelChanged(unsigned int backlightNum, unsigned int level);
    void profileChanged(QString profileID);
    void powerStateChanged(bool onACPower);

private slots:
    void pwrdRead();

protected:
     QPWRDClientPrivate * const d_ptr;
     //QPWRDClient(QPWRDClientPrivate &&d, QObject *parent) :QObject(parent) ,d_ptr(new QPWRDClientPrivate()){};
};



#endif // LIBPWRD_H
