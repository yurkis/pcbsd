#ifndef QPWRDEVENTS_H
#define QPWRDEVENTS_H

#include <QString>
#include <QVector>

#include "pwrdtypes.h"

class QPWRDEventsPrivate;

class QPWRDEvents:public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QPWRDEvents)
public:
    explicit QPWRDEvents(QObject *parent=0);
    ~QPWRDEvents();

    virtual bool connect(QString pipe = QString(DEF_PWRD_EVENTS_PIPE_NAME));
    virtual void disconnect();

signals:
    void backlightChanged(int backlight, int value);
    void batteryCapacityChanged(int batt, PWRBatteryStatus stat);
    void batteryStateChanged(int bat, PWRBatteryStatus stat);
    void acLineStateChanged(bool onExternalPower);
    void profileChanged(QString profileID);

protected:
     QPWRDEventsPrivate * const d_ptr;
};

#endif // QPWRDEVENTS_H

