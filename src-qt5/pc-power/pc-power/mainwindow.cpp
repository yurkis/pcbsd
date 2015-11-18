#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "widgets/widgetbacklight.h"
#include "widgets/widgetbattery.h"
#include "widgets/widgetsleepbuttons.h"

#include <QSystemTrayIcon>
#include <QMenu>
#include <QWidgetAction>
#include <QIcon>
#include <QPixmap>
#include <QPainter>
#include <QPicture>
#include <QDebug>


#define _str_constant static const char* const

_str_constant BASE_BATTERY_ICON = ":/images/battery.png";
_str_constant GOOD_BATTERY_ICON = ":/images/battery_good.png";
_str_constant LOW_BATTERY_ICON = ":/images/battery_low.png";
_str_constant CHARGING_IMAGE = ":/images/charging.png";
_str_constant AC_ENABLED_IMAGE = ":/images/ac_power.png";
_str_constant AC_DISABLED_IMAGE = ":/images/batt_power.png";
_str_constant NO_BATTERY_IMAGE = ":/images/no_battery.png";

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    client = new QPWRDClient(this);
    client->connect();
    events = new QPWRDEvents(this);
    events->connect();    

    connect(events, SIGNAL(backlightChanged(int,int)), this, SLOT(backlightChanged(int,int)));
    connect(events, SIGNAL(batteryCapacityChanged(int,PWRBatteryStatus)), this, SLOT(batteryCapacityChanged(int,PWRBatteryStatus)));
    connect(events, SIGNAL(batteryStateChanged(int,PWRBatteryStatus)), this, SLOT(batteryStateChanged(int,PWRBatteryStatus)));
    connect(events, SIGNAL(acLineStateChanged(bool)), this, SLOT(acLineStateChanged(bool)));
    connect(events, SIGNAL(profileChanged(QString)), this, SLOT(profileChanged(QString)));

    trayBattNo = 0;

    getInfoAndState();

    //Make profiles popup
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

    setupTray();

    refreshMainPageAcState();

    setupMainGeneral();

}

MainWindow::~MainWindow()
{
    delete ui;
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
    connect(show_act, SIGNAL(triggered(bool)), this, SLOT(showMainUI()));
    trayMenu->addAction(show_act);

    trayMenu->addMenu(profilesMenu);

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
    ui->acStateLabel->setPixmap(pixmap);// (QPicture((onACPower)?AC_ENABLED_IMAGE:AC_DISABLED_IMAGE));
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

}

void MainWindow::backlightChanged(int backlight, int value)
{
    qDebug()<<"blc: "<<backlight<<" "<<value<<"%";
}

void MainWindow::batteryCapacityChanged(int batt, PWRBatteryStatus stat)
{;
    qDebug()<<"batt cap "<<batt<<" cap:"<<stat.batteryCapacity<<" "<<stat.batteryCritical;
    if (batt == trayBattNo) refreshTrayIcon(stat);
}

void MainWindow::batteryStateChanged(int bat, PWRBatteryStatus stat)
{
    qDebug()<<"batt "<<bat<<" state changed to "<<stat.batteryState<<" "<<stat.batteryCritical;
    if (bat == trayBattNo) refreshTrayIcon(stat);
}

void MainWindow::acLineStateChanged(bool onExternalPower)
{
    qDebug()<<"AC power: "<<onExternalPower;
    onACPower = onExternalPower;

    ui->sysStateLabel->setText( (onACPower)?tr("On external power"):tr("On battery"));

    refreshMainPageAcState();

    QVector<PWRBatteryStatus> stats;
    if (client)
    {
        if (client->getBatteriesState(stats))
        {
            if (trayBattNo < stats.size())
            {
                refreshTrayIcon(stats[trayBattNo]);
            }
        }
    }
}

void MainWindow::profileChanged(QString profileID)
{
    qDebug()<<"profile changed to "<<profileID;
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
