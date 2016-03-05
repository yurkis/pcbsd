#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "widgets/widgetbacklight.h"
#include "widgets/widgetbattery.h"
#include "widgets/widgetsleepbuttons.h"
#include "widgets/widgetbatteryhw.h"

#include "dialogs/connecterrordialog.h"
#include "dialogs/profileeditdialog.h"

#include <QSystemTrayIcon>
#include <QMenu>
#include <QWidgetAction>
#include <QIcon>
#include <QPixmap>
#include <QPainter>
#include <QPicture>
#include <QMessageBox>
#include <QDebug>
#include <QListWidgetItem>

#define _str_constant static const char* const

_str_constant BASE_BATTERY_ICON = ":/images/battery.png";
_str_constant GOOD_BATTERY_ICON = ":/images/battery_good.png";
_str_constant LOW_BATTERY_ICON = ":/images/battery_low.png";
_str_constant CHARGING_IMAGE = ":/images/charging.png";
_str_constant AC_ENABLED_IMAGE = ":/images/ac_power.png";
_str_constant AC_DISABLED_IMAGE = ":/images/batt_power.png";
_str_constant NO_BATTERY_IMAGE = ":/images/no_battery.png";
_str_constant SETTINGS_WINDOW_ICON = ":/images/page-settings.png";
_str_constant EXIT_TRAY_ICON = ":/images/application-exit.png";
//_str_constant PROFILES_ICON = ":/images/page-profiles.png";

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    client = new QPWRDClient(this);
    events = new QPWRDEvents(this);

    if (!client->connect())
    {
        ConnectErrorDialog* dlg = new ConnectErrorDialog(this);
        dlg->execute(client, events);
        /*QMessageBox msgbox;
        msgbox.setText(tr("pwrd connection error. Please check if pwrd running"));
        msgbox.setWindowTitle(tr("error"));
        msgbox.setIcon(QMessageBox::Critical);
        msgbox.exec();
        exit(1);*/
    }

    if (!events->connect())
    {
        ConnectErrorDialog* dlg = new ConnectErrorDialog(this);
        dlg->execute(client, events);
    }

    connect(client, SIGNAL(connectionError()), this, SLOT(on_pwrd_connectionError()));

    connect(events, SIGNAL(backlightChanged(int,int)), this, SLOT(backlightChanged(int,int)));
    connect(events, SIGNAL(batteryCapacityChanged(int,PWRBatteryStatus)), this, SLOT(batteryCapacityChanged(int,PWRBatteryStatus)));
    connect(events, SIGNAL(batteryStateChanged(int,PWRBatteryStatus)), this, SLOT(batteryStateChanged(int,PWRBatteryStatus)));
    connect(events, SIGNAL(acLineStateChanged(bool)), this, SLOT(acLineStateChanged(bool)));
    connect(events, SIGNAL(profileChanged(QString)), this, SLOT(profileChanged(QString)));
    connect(events, SIGNAL(buttonsStateChanged(QString,QString,QString)), this, SLOT(buttonsStateChanged(QString,QString,QString)));
    trayBattNo = 0;

    profilesMenu = NULL;

    getInfoAndState();
    initUI();

    notification = new Notification;
    notification->setup(events, client);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    hide();
    event->ignore();
}

void MainWindow::getInfoAndState()
{
    if (!client->getHardwareInfo(hwInfo))
    {
        //TODO: error message
    }
    if (!client->getACLineState(onACPower))
    {
        //TODO: error message
    }
    if (!client->getProfiles(profiles))
    {
        //TODO: error message
    }
    if (!client->getCurrentProfileID(currentProfile))
    {
        //TODO: error message
    }
    if (!client->getActiveProfiles(&acProffile, &battProfile, &lowbattProfile))
    {
        //TODO: error message
    }
    if (!client->getBatteriesState(battStates))
    {
        //TODO: error message
    }
    if (!client->getDaemonSettings(daemonSettings))
    {
        //TODO: error message
    }
}

void MainWindow::initUI()
{
    refreshProfilesMenu();

    //fill available sleep states descriptions
    SSleepStateDescription desc;
    desc.description = tr("None");
    desc.state = "NONE";
    ssDescriptions.push_back(desc);
    if (hwInfo.basic.possibleACPIStates.contains("S3"))
    {
        desc.description = tr("Sleep");
        desc.state="S3";
        ssDescriptions.push_back(desc);
    }
    if (hwInfo.basic.possibleACPIStates.contains("S4"))
    {
        desc.description = tr("Hibernate");
        desc.state="S4";
        ssDescriptions.push_back(desc);
    }
    if (hwInfo.basic.possibleACPIStates.contains("S5"))
    {
        desc.description = tr("Power off");
        desc.state="S5";
        ssDescriptions.push_back(desc);
    }

    setupTray();

    refreshMainPageAcState();

    setupMainGeneral();
    setupMainButtonsAndLid();

    setupInfo();
    setupProfiles();

    ui->mainStack->setCurrentIndex(0);
    ui->mainTW->setCurrentItem(ui->mainTW->topLevelItem(0));
}

void MainWindow::setupTray()
{
    trayIcon = new QSystemTrayIcon(this);

    if (!hwInfo.basic.numBatteries)
    {
        trayIcon->setIcon(QIcon(NO_BATTERY_IMAGE));
    }
    else
    {
        if (trayBattNo>=battStates.size())
            trayBattNo = 0;
        refreshTrayIcon(battStates[trayBattNo]);
    }

    trayMenu = new QMenu(this);

    trayIcon->setContextMenu(trayMenu);

    QAction* show_act = new  QAction(trayMenu);
    show_act->setText(tr("Show main window"));
    show_act->setIcon(QIcon(SETTINGS_WINDOW_ICON));
    connect(show_act, SIGNAL(triggered(bool)), this, SLOT(showMainUI()));
    trayMenu->addAction(show_act);

    trayMenu->addMenu(profilesMenu);

    trayMenu->addSeparator();

    QAction* exit_action = new QAction(trayMenu);
    exit_action->setText(tr("Close tray"));
    exit_action->setIcon(QIcon(EXIT_TRAY_ICON));
    connect(exit_action, SIGNAL(triggered(bool)), this, SLOT(on_actionExit_triggered()));
    trayMenu->addAction(exit_action);

    trayMenu->addSeparator();

    for(int i=hwInfo.basic.numBatteries - 1; i>=0; i--)
    {
        WidgetBattery* batt_widget = new WidgetBattery (trayMenu);
        batt_widget->setup(i, client, events);
        QWidgetAction *bw_action= new QWidgetAction(trayMenu);
        bw_action->setDefaultWidget(batt_widget);
        trayMenu->addAction(bw_action);
        trayMenu->addSeparator();
    }


    trayMenu->addSeparator();
    WidgetSleepButtons* sbw = new WidgetSleepButtons(trayMenu);
    if (sbw->setup(client, hwInfo.basic.possibleACPIStates))
    {
        QWidgetAction *sbw_action= new QWidgetAction(trayMenu);
        sbw_action->setDefaultWidget(sbw);
        trayMenu->addAction(sbw_action);
    }
    else
    {
        delete sbw;
    }

    trayMenu->addSeparator();

    for(int i=hwInfo.basic.numBacklights - 1; i>=0; i--)
    {
        WidgetBacklight* bl_widget = new WidgetBacklight (trayMenu);
        bl_widget->setup(i, client, events);
        QWidgetAction *bl_action= new QWidgetAction(trayMenu);
        bl_action->setDefaultWidget(bl_widget);
        trayMenu->addAction(bl_action);
    }

     trayIcon->show();


     connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayActivated()));
}

void MainWindow::refreshProfilesMenu()
{
    //Make profiles popup
    if (profilesMenu)
        profilesMenu->clear();
    else
        profilesMenu = new QMenu(this);
    for (int i=0;i<profiles.size();i++)
    {
        QAction* action = new QAction(profilesMenu);
        action->setText(profiles[i].name);
        action->setData(QVariant(profiles[i].id));
        connect(action, SIGNAL(triggered(bool)), this, SLOT(changeProfileTriggered()));
        profilesMenu->addAction(action);
    }
    profilesMenu->setTitle(tr("Change profile"));
}

void MainWindow::refreshTrayIcon(PWRBatteryStatus stat)
{
    static bool last_power= false;
    static unsigned int last_cap = -1;
    static bool was_low = false;
    static int batt_redraw_prcentage = 5; // Not always repaint battery image. Reduce CPU cycles (power consumption)

    int delta = (last_cap > stat.batteryCapacity)?last_cap - stat.batteryCapacity: stat.batteryCapacity - last_cap;

    bool need_update = trayIconImage.isNull();
    need_update = need_update || ( delta >=batt_redraw_prcentage );
    need_update = need_update || (last_power != onACPower);
    need_update = need_update || (was_low != stat.batteryCritical);

    if (!need_update) return;
    //if we need to repaint battery image
    last_cap = stat.batteryCapacity;
    last_power = onACPower;

    //Get clean battery pixmap...
    QPixmap icon_pixmap;
    icon_pixmap.load(BASE_BATTERY_ICON);
    int icon_w = icon_pixmap.width();
    int icon_h = icon_pixmap.height();
    QPainter painter(&icon_pixmap);

    //Get capacity pixmap (different pixmaps for normal battery state and low battery state)
    QString cap_pixmap_name = (stat.batteryCritical)?LOW_BATTERY_ICON:GOOD_BATTERY_ICON;

    //...and then cut capacity pixmap due to capacity percentage, center it
    // and draw on the clean battery pixmap
    QPixmap cap_pixmap;
    cap_pixmap.load(cap_pixmap_name);
    int cap_w = cap_pixmap.width();
    int cap_h = cap_pixmap.height();
    int cut_h = (int)(((float)cap_h) * (((float)stat.batteryCapacity) / 100.));

    QPoint draw_point((icon_w - cap_w)/2, (icon_h - cap_h)/2 + cap_h - cut_h);

    QRect cut_rect=QRect(0, cap_h - cut_h ,cap_w, cut_h);

    painter.drawPixmap(draw_point, cap_pixmap, cut_rect);

    batt_redraw_prcentage =  (int)(.1 * cap_h); // calculate how much percents in one capacity line

    if (onACPower)
    {
        //... paint 'external power' image on the left bottom corner if needs
        QPixmap charging_pixmap;
        charging_pixmap.load(CHARGING_IMAGE);
        int h = charging_pixmap.height();
        int w = charging_pixmap.width();

        painter.drawPixmap(icon_w - w, icon_h - h, charging_pixmap);
    }

    trayIconImage = QIcon(icon_pixmap);

    trayIcon->setIcon(trayIconImage);
}

void MainWindow::refreshMainPageAcState()
{
    QPixmap pixmap;
    pixmap.load((onACPower)?AC_ENABLED_IMAGE:AC_DISABLED_IMAGE);
    ui->acStateLabel->setPixmap(pixmap);
}

void MainWindow::refreshPowerCosumption()
{
    int pc = powerConsumption();
    ui->powerConsumptionPB->setVisible(pc);
    if (pc)
    {
        ui->powerConsumptionLabel->setText(tr("%1 (mW)").arg(pc));
        if (ui->powerConsumptionPB->maximum() < pc)
            ui->powerConsumptionPB->setMaximum(pc);
        ui->powerConsumptionPB->setValue(pc);
    }
    else
    {
        ui->powerConsumptionLabel->setText(tr("Unknown"));
    }
}

void MainWindow::setupMainGeneral()
{
    ui->sysStateLabel->setText( (onACPower)?tr("On external power"):tr("On battery"));
    ui->currProfileLabel->setText(currentProfile.name);
    ui->changeProfileButton->setMenu(profilesMenu);

    QLayout* layout = ui->homePAgeMainLayout->layout();

    for(int i=hwInfo.basic.numBatteries - 1; i>=0; i--)
    {
        WidgetBattery* batt_widget = new WidgetBattery (this);
        batt_widget->setup(i, client, events);
        layout->addWidget(batt_widget);
    }

    for(int i=hwInfo.basic.numBacklights - 1; i>=0; i--)
    {
        WidgetBacklight* bl_widget = new WidgetBacklight (this);
        bl_widget->setup(i, client, events);
        layout->addWidget(bl_widget);
    }

    WidgetSleepButtons* sbw = new WidgetSleepButtons(this);
    if (sbw->setup(client, hwInfo.basic.possibleACPIStates))
    {
        layout->addWidget(sbw);
    }
    else
    {
        delete sbw;
    }

    refreshPowerCosumption();

}

void MainWindow::setupInfo()
{
    ui->numBacklightsLabel->setText(QString::number(hwInfo.basic.numBacklights));
    ui->numBatteriesLabel->setText(QString::number(hwInfo.basic.numBatteries));
    QString states;
    for (int i=0; i<hwInfo.basic.possibleACPIStates.size(); i++)
    {
        if (i) states+=", ";
        if (hwInfo.basic.possibleACPIStates[i] == "S3")
        {
            states+=tr("Sleep");
        }
        else if (hwInfo.basic.possibleACPIStates[i] == "S4")
        {
            states+=tr("Hibernate");
        }
        else if (hwInfo.basic.possibleACPIStates[i] == "S5")
        {
            states+=tr("Power off");
        }
        else
        {
            states+=hwInfo.basic.possibleACPIStates[i];
        }

    }
    ui->sleepStatesLabel->setText(states);

    for(int i=0; i<hwInfo.batteries.size(); i++)
    {
        WidgetBatteryHW* widget = new WidgetBatteryHW(this);
        widget->setup(i, client);
        ui->infoTab->addTab(widget, tr("Battery %1").arg(QString::number(i)));
    }
}

int MainWindow::powerConsumption()
{
    for(int i=0; i<battStates.size(); i++)
    {
        if ((battStates[i].batteryState == BATT_DISCHARGING) && (battStates[i].powerConsumption))
        {
            return battStates[i].powerConsumption;
        }
    }
    return 0;
}

void MainWindow::setupMainButtonsAndLid()
{
    ui->powerBtnCB->clear();
    ui->sleepBtnCB->clear();
    ui->lidCB->clear();

    for (int i=0;i<ssDescriptions.size();i++)
    {
        ui->powerBtnCB->addItem(ssDescriptions[i].description);
        ui->sleepBtnCB->addItem(ssDescriptions[i].description);
        ui->lidCB->addItem(ssDescriptions[i].description);
    }
    bool isshow = hwInfo.basic.hasSleepButton;
    ui->sleepBtnCB->setVisible(isshow);
    ui->sleepBtnDesc->setVisible(isshow);
    ui->sleepBtnImg->setVisible(isshow);


    isshow = hwInfo.basic.hasLid;
    ui->lidCB->setVisible(isshow);
    ui->lidDesc->setVisible(isshow);
    ui->lidImg->setVisible(isshow);

    QString power,sleep,lid;

    if (!client) return;
    if (!client->getButtonsState(power,sleep,lid)) return;

    refreshButtonsAndLid(power,sleep,lid);

    connect(ui->lidCB, SIGNAL(currentIndexChanged(int)), this, SLOT(btnIndexChanged()));
    connect(ui->sleepBtnCB, SIGNAL(currentIndexChanged(int)), this, SLOT(btnIndexChanged()));
    connect(ui->powerBtnCB, SIGNAL(currentIndexChanged(int)), this, SLOT(btnIndexChanged()));
}

void MainWindow::refreshButtonsAndLid(QString power, QString sleep, QString lid)
{
    if (!client) return;
    if (!client->getButtonsState(power,sleep,lid)) return;    

    for(int i=0; i<ssDescriptions.size(); i++)
    {
        if (power == ssDescriptions[i].state)
        {
            ui->powerBtnCB->setCurrentIndex(i);
            break;
        }
    }//for sleep mode descriptions

    if (hwInfo.basic.hasSleepButton)
    {
        for(int i=0; i<ssDescriptions.size(); i++)
        {
            if (sleep == ssDescriptions[i].state)
            {
                ui->sleepBtnCB->setCurrentIndex(i);
                break;
            }
        }//for sleep mode descriptions
    }//if sleep button present
    if (hwInfo.basic.hasLid)
    {
        for(int i=0; i<ssDescriptions.size(); i++)
        {
            if (lid == ssDescriptions[i].state)
            {
                ui->lidCB->setCurrentIndex(i);
                break;
            }
        }//for sleep mode descriptions
    }//if sleep button present
}

void MainWindow::setupProfiles()
{
    ui->onACPowerProfile->clear();
    ui->onBatteryProfile->clear();
    ui->onLowBatteryProfile->clear();

    for(int i=0; i<profiles.size(); i++)
    {
        ui->onACPowerProfile->addItem(profiles[i].name);
        if (acProffile.id == profiles[i].id) ui->onACPowerProfile->setCurrentIndex(i);
        ui->onBatteryProfile->addItem(profiles[i].name);
        if(battProfile.id == profiles[i].id) ui->onBatteryProfile->setCurrentIndex(i);
        ui->onLowBatteryProfile->addItem(profiles[i].name);
        if(lowbattProfile.id == profiles[i].id) ui->onLowBatteryProfile->setCurrentIndex(i);
    }

    refreshProfilesList();
}

void MainWindow::refreshProfilesList()
{
    ui->profilesLW->clear();
    for (int i=0; i<profiles.size(); i++)
    {
        QListWidgetItem* item = new QListWidgetItem(ui->profilesLW);
        item->setText(profiles[i].name);
        item->setData(Qt::UserRole, profiles[i].id);
        ui->profilesLW->addItem(item);

        if (!i) ui->profilesLW->setCurrentItem(item);
    }

}

void MainWindow::backlightChanged(int backlight, int value)
{

}

void MainWindow::batteryCapacityChanged(int batt, PWRBatteryStatus stat)
{;
    if (battStates.size())
        battStates[batt] = stat;
    if (batt == trayBattNo) refreshTrayIcon(stat);

    refreshPowerCosumption();
}

void MainWindow::batteryStateChanged(int bat, PWRBatteryStatus stat)
{
    if (battStates.size())
        battStates[bat] = stat;
    if (bat == trayBattNo) refreshTrayIcon(stat);

    refreshPowerCosumption();
}

void MainWindow::acLineStateChanged(bool onExternalPower)
{    
    onACPower = onExternalPower;

    ui->sysStateLabel->setText( (onACPower)?tr("On external power"):tr("On battery"));

    refreshMainPageAcState();

    if (client)
    {
        if (client->getBatteriesState(battStates))
        {
            if (trayBattNo < battStates.size())
            {
                refreshTrayIcon(battStates[trayBattNo]);
            }
        }//if got states
    }//if valid client

    refreshPowerCosumption();
}

void MainWindow::profileChanged(QString profileID)
{    
    for (int i=0; i<profiles.size(); i++)
    {
        if (profiles[i].id == profileID)
        {
            ui->currProfileLabel->setText(profiles[i].name);
        }
    }
    //ui->currProfileLabel->setText(currentProfile.name);
}

void MainWindow::trayActivated()
{
    trayMenu->popup(QCursor::pos());
}

void MainWindow::showMainUI()
{
    showNormal();
}

void MainWindow::changeProfileTriggered()
{
    if (!client) return;
    QObject* obj = sender();
    if (!obj) return;
    QAction* src;
    try
    {
        src = dynamic_cast<QAction*>(obj);
    }
    catch(...)
    {
        return;
    }

    client->setCurrentProfile(src->data().toString());
}

void MainWindow::btnIndexChanged()
{
    ui->applyBtnSettings->setEnabled(true);
}

void MainWindow::buttonsStateChanged(QString powerBtnState, QString sleepBtnState, QString lidSwitchState)
{
    refreshButtonsAndLid(powerBtnState, sleepBtnState, lidSwitchState);    
}

void MainWindow::on_actionCloseWindow_triggered()
{
    hide();
}

void MainWindow::on_actionExit_triggered()
{
    QApplication::exit();
}

void MainWindow::on_applyBtnSettings_clicked()
{
    if (!client) return;

    QString power="NONE";
    QString sleep="NONE";
    QString lid="NONE";
    if (ui->powerBtnCB->currentIndex() < ssDescriptions.size())
        power = ssDescriptions[ui->powerBtnCB->currentIndex()].state;
    if (ui->sleepBtnCB->currentIndex() < ssDescriptions.size())
        sleep = ssDescriptions[ui->sleepBtnCB->currentIndex()].state;
    if (ui->lidCB->currentIndex() < ssDescriptions.size())
        lid = ssDescriptions[ui->lidCB->currentIndex()].state;

    if (!client->setButtonsState(&power, (hwInfo.basic.hasSleepButton)?&sleep:NULL, (hwInfo.basic.hasLid)?&lid:NULL))
    {
        //TODO: error message
    }
    ui->applyBtnSettings->setEnabled(false);
}

void MainWindow::on_mainTW_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    Q_UNUSED(previous);
    for (int i=0; i<ui->mainTW->topLevelItemCount(); i++)
    {
        if (ui->mainTW->topLevelItem(i) == current)
        {
            ui->mainStack->setCurrentIndex(i);
            break;
        }
    }
}

void MainWindow::on_pwrd_connectionError()
{
    ConnectErrorDialog* dlg = new ConnectErrorDialog(this);
    dlg->execute(client, events);

    getInfoAndState();

    QString power, sleep, lid;

    if (client->getButtonsState(power,sleep,lid))
        refreshButtonsAndLid(power,sleep,lid);

    setupMainGeneral();
    refreshMainPageAcState();

    refreshProfilesMenu();
    if (client->getBatteriesState(battStates))
    {
        if (trayBattNo < battStates.size())
        {
            refreshTrayIcon(battStates[trayBattNo]);
        }
    }//if got states
}

void MainWindow::on_pwrdError(QString message)
{

}

void MainWindow::on_cuurProfilesSaveBtn_clicked()
{
    if (!client) return;
    if (!client->getDaemonSettings(daemonSettings))
        return;

    if (profiles.size()>ui->onACPowerProfile->currentIndex())
        daemonSettings.onACProfile = profiles[ui->onACPowerProfile->currentIndex()].id;
    if (profiles.size()>ui->onBatteryProfile->currentIndex())
        daemonSettings.onBatteryProfile = profiles[ui->onBatteryProfile->currentIndex()].id;
    if (profiles.size()>ui->onLowBatteryProfile->currentIndex())
        daemonSettings.onLowBatteryProfile = profiles[ui->onLowBatteryProfile->currentIndex()].id;

    client->setDaemonSettings(daemonSettings);

    setupProfiles();
}

void MainWindow::on_addProfileBtn_clicked()
{
    ProfileEditDialog* dlg = new ProfileEditDialog(this);
    dlg->setup(client, ssDescriptions);
    dlg->exec();
}

void MainWindow::on_editProfileBtn_clicked()
{
    ProfileEditDialog* dlg = new ProfileEditDialog(this);
/* for (int i=0; i<profiles.size(); i++)
    {
        QListWidgetItem* item = new QListWidgetItem(ui->profilesLW);
        item->setText(profiles[i].name);
        item->setData(Qt::UserRole, profiles[i].id);
        ui->profilesLW->addItem(item);

        if (!i) ui->profilesLW->setCurrentItem(item);
    }*/
    QListWidgetItem* item = ui->profilesLW->currentItem();
    dlg->setup(item->data(Qt::UserRole).toString(), client, ssDescriptions);
    dlg->exec();
}
