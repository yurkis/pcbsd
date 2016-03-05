#ifndef QPWRDEVENTS_H
#define QPWRDEVENTS_H

#include <QString>
#include <QVector>
#include <QObject>

#include "pwrdtypes.h"

class QPWRDEventsPrivate;

//! PWRD daemon events handler class
class QPWRDEvents:public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QPWRDEvents)
public:
    explicit QPWRDEvents(QObject *parent=0);
    ~QPWRDEvents();

    //! Connect to pwrd server
    /*!
     * \param pipe- full path to pwrd events pipe
     *
     * \return true if success
     */
    virtual bool connect(QString pipe = QString(DEF_PWRD_EVENTS_PIPE_NAME));

    //! Disconnect from pwrd
    virtual void disconnect();

signals:

    //! Backlight level changed event
    /*!
     * \param backlight - backlight number
     * \param value - new backlight level value
     */
    void backlightChanged(int backlight, int value);

    //! Battery capacity changed event
    /*!
     * Emmited by pwrd each time when battery capacity changes by 1%
     * \param batt - battery number
     * \param stat - battery state
     */
    void batteryCapacityChanged(int batt, PWRBatteryStatus stat);

    //! Battery state changed event
    /*!
     * Calls each time when battery state (charging / discharging) changes
     * \param bat - battery number
     * \param stat - battery state
     */
    void batteryStateChanged(int bat, PWRBatteryStatus stat);

    //! AC line state changed
    /*!
     * Calls by each AC line state change (external power / AC power)
     * \param onExternalPower - true if device is on external power
     */
    void acLineStateChanged(bool onExternalPower);

    //! Current profile changed
    /*!
     * Calls when current profile is changed
     * \param profileID - new profile ID
     */
    void profileChanged(QString profileID);

    //! Buttons sleep states changed
    /*!
     * \param powerBtnState - new state for 'Power' button
     * \param sleepBtnState - new state for 'Sleep' button
     * \param lidSwitchState - new state for lid switch
     */
    void buttonsStateChanged(QString powerBtnState, QString sleepBtnState, QString lidSwitchState);

protected:
     QPWRDEventsPrivate * const d_ptr;
};

#endif // QPWRDEVENTS_H

