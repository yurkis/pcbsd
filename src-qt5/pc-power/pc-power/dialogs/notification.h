#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#include <QDialog>
#include <QTimer>

#include <QPWRDEvents.h>

namespace Ui {
class Notification;
}

class Notification : public QDialog
{
    Q_OBJECT

public:
    explicit Notification(QWidget *parent = 0);
    ~Notification();

    void setup(QPWRDEvents* _ev);

private slots:
    void backlightChanged(int backlight, int value);
    void batteryStateChanged(int bat, PWRBatteryStatus stat);
    void acLineStateChanged(bool onExternalPower);
    void profileChanged(QString profileID);

private:
    Ui::Notification *ui;

    QPWRDEvents* ev;
    QTimer* timer;
};

#endif // NOTIFICATION_H
