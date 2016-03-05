#ifndef WIDGETBACKLIGHT_H
#define WIDGETBACKLIGHT_H

#include <QWidget>

#include <QPWRDClient.h>
#include <QPWRDEvents.h>

namespace Ui {
class WidgetBacklight;
}

class WidgetBacklight : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetBacklight(QWidget *parent = 0);
    ~WidgetBacklight();

    void setup(int num, QPWRDClient* cl, QPWRDEvents* ev, int value =-1);
    void setCurrValue(int num);
    int value();
    void setValue(int val);

private slots:
    void pwrdValueChanged(int backlight, int value);

    void on_level_sliderMoved(int position);

    void on_level_valueChanged(int value);

    void on_level_sliderReleased();

    //void on_level_sliderPressed();

private:
    int blNum;
    QPWRDClient* client;
    QPWRDEvents* events;

    bool ignoreEvents;
    int eventsToIgnore;
    Ui::WidgetBacklight *ui;

    void refreshUI(int value);
};

#endif // WIDGETBACKLIGHT_H
