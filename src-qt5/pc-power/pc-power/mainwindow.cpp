#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "widgets/widgetbacklight.h"

#include <QSystemTrayIcon>
#include <QMenu>
#include <QWidgetAction>
#include <QIcon>
#include <QPixmap>
#include <QPainter>
#include <QDebug>


#define _str_constant static const char* const

_str_constant BASE_BATTERY_ICON = ":/images/battery.png";
_str_constant GOOD_BATTERY_ICON = ":/images/battery_good.png";
_str_constant CHARGING_IMAGE = ":/images/charging.png";
const int BATTERY_REDRAW_PERCENTAGE = 5;

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

    setupTray();

    ui->testWidget->setup(0, client, events);
    ui->test2->setup(0, client, events);    



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

    refreshTrayIcon(battStates[trayBattNo]);

    trayMenu = new QMenu(this);

    trayIcon->setContextMenu(trayMenu);
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

    //if (hwInfo.basic.numBatteries) trayMenu->addSeparator();

    for(int i=hwInfo.basic.numBacklights - 1; i>=0; i--)
    {
        WidgetBacklight* bl_widget = new WidgetBacklight (trayMenu);
        bl_widget->setup(i, client, events);
        QWidgetAction *bl_action= new QWidgetAction(trayMenu);
        bl_action->setDefaultWidget(bl_widget);
        trayMenu->addAction(bl_action);
    }

     trayIcon->show();

}

void MainWindow::refreshTrayIcon(PWRBatteryStatus stat)
{
    static bool last_power= false;
    static int last_cap = -1;

    int delta = (last_cap > stat.batteryCapacity)?last_cap - stat.batteryCapacity: stat.batteryCapacity - last_cap;

    bool need_update = trayIconImage.isNull();
    need_update = need_update || ( delta >=BATTERY_REDRAW_PERCENTAGE );
    need_update = need_update || (last_power != onACPower);

    if (!need_update) return;

    last_cap = stat.batteryCapacity;
    last_power = onACPower;

    QPixmap icon_pixmap;
    icon_pixmap.load(BASE_BATTERY_ICON);
    int icon_w = icon_pixmap.width();
    int icon_h = icon_pixmap.height();
    QPainter painter(&icon_pixmap);

    QString cap_pixmap_name = GOOD_BATTERY_ICON;

    QPixmap cap_pixmap;
    cap_pixmap.load(cap_pixmap_name);
    int cap_w = cap_pixmap.width();
    int cap_h = cap_pixmap.height();
    int cut_h = (int)(((float)cap_h) * (((float)stat.batteryCapacity) / 100.));

    QPoint draw_point((icon_w - cap_w)/2, (icon_h - cap_h)/2 + cap_h - cut_h);

    QRect cut_rect=QRect(0, cap_h - cut_h ,cap_w, cut_h);

    painter.drawPixmap(draw_point, cap_pixmap, cut_rect);

    if (onACPower)
    {
        QPixmap charging_pixmap;
        charging_pixmap.load(CHARGING_IMAGE);
        int h = charging_pixmap.height();
        int w = charging_pixmap.width();

        painter.drawPixmap(icon_w - w, icon_h - h, charging_pixmap);
    }

    trayIconImage = QIcon(icon_pixmap);

    trayIcon->setIcon(trayIconImage);

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
}
