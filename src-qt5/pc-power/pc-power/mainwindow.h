#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QVector>
#include <QCloseEvent>
#include <QTreeWidgetItem>

#include "QPWRDClient.h"
#include "QPWRDEvents.h"

#include "ssdescription.h"

#include "dialogs/notification.h"

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

    PWRDHardwareInfo hwInfo;
    QVector<PWRBatteryStatus> battStates;
    bool onACPower;
    PWRProfileInfoBasic currentProfile;
    QVector<PWRProfileInfoBasic> profiles;
    PWRProfileInfoBasic acProffile;
    PWRProfileInfoBasic battProfile;
    PWRProfileInfoBasic lowbattProfile;

    PWRDaemonSettings daemonSettings;

    QSystemTrayIcon* trayIcon;
    QIcon trayIconImage;
    QMenu* trayMenu;

    QMenu* profilesMenu;

    int trayBattNo;

    QVector<SSleepStateDescription> ssDescriptions;

    Notification* notification;

    virtual void closeEvent(QCloseEvent *event);

protected:
    void getInfoAndState();
    void initUI();
    void setupTray();
    void refreshProfilesMenu();
    void refreshTrayIcon(PWRBatteryStatus stat);
    void refreshMainPageAcState();
    void refreshPowerCosumption();
    void setupMainGeneral();
    void setupInfo();
    int powerConsumption();
    void setupMainButtonsAndLid();
    void refreshButtonsAndLid(QString power, QString sleep, QString lid);
    void setupProfiles();
    void refreshProfilesList();

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

    void on_pwrd_connectionError();

    void on_pwrdError(QString message);

    void on_cuurProfilesSaveBtn_clicked();

    void on_addProfileBtn_clicked();

    void on_editProfileBtn_clicked();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
