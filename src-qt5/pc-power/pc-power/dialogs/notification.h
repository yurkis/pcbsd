#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#include <QDialog>
#include <QTimer>

#include <QPWRDEvents.h>
#include <QPWRDClient.h>

namespace Ui {
class Notification;
}

class Notification : public QDialog
{
    Q_OBJECT

public:
    explicit Notification(QWidget *parent = 0);
    ~Notification();

    void setup(QPWRDEvents* _ev, QPWRDClient* _cl);

private slots:
    void backlightChanged(int backlight, int value);
    void batteryStateChanged(int bat, PWRBatteryStatus stat);
    void acLineStateChanged(bool onExternalPower);
    void profileChanged(QString profileID);

    void hideMe();

private:
    Ui::Notification *ui;

    QPWRDEvents* ev;
    QPWRDClient* cl;
    QTimer* timer;

    typedef enum{
        eCN_AC,
        eCN_BATT_CRITICAL,
        eCN_PROFILE,
        eCN_BACKLIGHT,
        eCN_NONE
    }ECurrentNotification;

    ECurrentNotification currNotification;

    bool notify(ECurrentNotification level, int page_no);
};

#endif // NOTIFICATION_H
