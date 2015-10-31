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
    virtual bool getHardwareInfo(PWRDHardwareInfo &out);
    virtual int getBacklightLevel(int backlight = 0);
    virtual bool setBacklightLevel(int level, int backlight = PWR_ALL);
    virtual bool setBacklightLevelRelative(int level, int backlight = PWR_ALL);
    virtual bool setBacklightLevel(QString level, int backlight = PWR_ALL);

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
