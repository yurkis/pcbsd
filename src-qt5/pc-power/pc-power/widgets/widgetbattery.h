#ifndef WIDGETBATTERY_H
#define WIDGETBATTERY_H

#include <QWidget>

namespace Ui {
class WidgetBattery;
}

class WidgetBattery : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetBattery(QWidget *parent = 0);
    ~WidgetBattery();

private:
    Ui::WidgetBattery *ui;
};

#endif // WIDGETBATTERY_H
