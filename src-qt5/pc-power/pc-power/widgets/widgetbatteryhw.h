#ifndef WIDGETBATTERYHW_H
#define WIDGETBATTERYHW_H

#include <QWidget>

namespace Ui {
class WidgetBatteryHW;
}

class WidgetBatteryHW : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetBatteryHW(QWidget *parent = 0);
    ~WidgetBatteryHW();

private:
    Ui::WidgetBatteryHW *ui;
};

#endif // WIDGETBATTERYHW_H
