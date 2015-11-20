#ifndef WIDGETBATTERYHW_H
#define WIDGETBATTERYHW_H

#include <QWidget>
#include "QPWRDClient.h"

namespace Ui {
class WidgetBatteryHW;
}

class WidgetBatteryHW : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetBatteryHW(QWidget *parent = 0);
    ~WidgetBatteryHW();

    bool setup(int batt_no, QPWRDClient* client);

private:
    Ui::WidgetBatteryHW *ui;

};

#endif // WIDGETBATTERYHW_H
