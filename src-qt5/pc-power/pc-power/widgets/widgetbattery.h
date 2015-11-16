#ifndef WIDGETBATTERY_H
#define WIDGETBATTERY_H

#include <QWidget>

#include <QPWRDClient.h>
#include <QPWRDEvents.h>

namespace Ui {
class WidgetBattery;
}

class WidgetBattery : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetBattery(QWidget *parent = 0);
    ~WidgetBattery();

    void setup(int num, QPWRDClient* cl, QPWRDEvents* ev);

private slots:
    void batteryChanged(int batt, PWRBatteryStatus stat);

private:
    int battNum;
    QPWRDClient* client;
    QPWRDEvents* events;

    void refreshUI(PWRBatteryStatus stat);

    Ui::WidgetBattery *ui;
};

#endif // WIDGETBATTERY_H
