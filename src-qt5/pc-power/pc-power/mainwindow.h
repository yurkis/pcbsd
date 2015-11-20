#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QVector>
#include <QCloseEvent>
#include <QTreeWidgetItem>

#include "QPWRDClient.h"
#include "QPWRDEvents.h"

namespace Ui {
class MainWindow;
}

typedef struct _SSleepStateDescription
{
    QString description;
    QString state;
}SSleepStateDescription;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    QPWRDClient* client;
    QPWRDEvents* events;

    PWRDHardwareInfo hwInfo;
    QVector<PWRBatteryStatus> battStates;
    bool onACPower;
    PWRProfileInfoBasic currentProfile;
    QVector<PWRProfileInfoBasic> profiles;
    PWRProfileInfoBasic acProffile;
    PWRProfileInfoBasic battProfile;
    PWRProfileInfoBasic lowbattProfile;

    QSystemTrayIcon* trayIcon;
    QIcon trayIconImage;
    QMenu* trayMenu;

    QMenu* profilesMenu;

    int trayBattNo;

    QVector<SSleepStateDescription> ssDescriptions;

    virtual void closeEvent(QCloseEvent *event);

protected:
    void getInfoAndState();
    void setupTray();
    void refreshTrayIcon(PWRBatteryStatus stat);
    void refreshMainPageAcState();
    void refreshPowerCosumption();
    void setupMainGeneral();
    void setupInfo();
    int powerConsumption();
    void setupMainButtonsAndLid();
    void refreshButtonsAndLid(QString power, QString sleep, QString lid);

public slots:
    void backlightChanged(int backlight, int value);
    void batteryCapacityChanged(int batt, PWRBatteryStatus stat);
    void batteryStateChanged(int bat, PWRBatteryStatus stat);
    void acLineStateChanged(bool onExternalPower);
    void profileChanged(QString profileID);
    void trayActivated();
    void showMainUI();
    void changeProfileTriggered();
    void btnIndexChanged();
    void buttonsStateChanged(QString powerBtnState, QString sleepBtnState, QString lidSwitchState);

private slots:
    void on_actionCloseWindow_triggered();

    void on_actionExit_triggered();

    void on_applyBtnSettings_clicked();

    void on_mainTW_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
