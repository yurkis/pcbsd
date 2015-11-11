#ifndef WIDGETBACKLIGHT_H
#define WIDGETBACKLIGHT_H

#include <QWidget>

namespace Ui {
class WidgetBacklight;
}

class WidgetBacklight : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetBacklight(QWidget *parent = 0);
    ~WidgetBacklight();

private:
    Ui::WidgetBacklight *ui;
};

#endif // WIDGETBACKLIGHT_H
