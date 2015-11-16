#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "QPWRDClient.h"
#include "QPWRDEvents.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    QPWRDClient* client;
    QPWRDEvents* events;

public slots:
    void backlightChanged(int backlight, int value);
    void batteryCapacityChanged(int batt, PWRBatteryStatus stat);
    void batteryStateChanged(int bat, PWRBatteryStatus stat);
    void acLineStateChanged(bool onExternalPower);
    void profileChanged(QString profileID);

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
