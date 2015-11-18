#ifndef WIDGETSLEEPBUTTONS_H
#define WIDGETSLEEPBUTTONS_H

#include <QWidget>
#include <QPWRDClient.h>

namespace Ui {
class WidgetSleepButtons;
}

class WidgetSleepButtons : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetSleepButtons(QWidget *parent = 0);
    ~WidgetSleepButtons();

    bool setup(QPWRDClient* cl, QStringList possibleACPIStates);

private slots:
    void on_sleepButton_clicked();

    void on_hibernateButton_clicked();

private:
    Ui::WidgetSleepButtons *ui;

    QPWRDClient* client;
};

#endif // WIDGETSLEEPBUTTONS_H
