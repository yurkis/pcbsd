#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "widgets/widgetbacklight.h"

#include <QSystemTrayIcon>
#include <QMenu>
#include <QWidgetAction>
#include <QDebug>


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

    ui->testWidget->setup(0, client, events);
    ui->test2->setup(0, client, events);

    QSystemTrayIcon* trayIcon = new QSystemTrayIcon(this);
    QMenu* trayIconMenu = new QMenu();
    trayIcon->setContextMenu(trayIconMenu);
    QAction *menuline = trayIconMenu->addSeparator();

    WidgetBattery* batt_test2 = new WidgetBattery(this);
    batt_test2->setup(0, client, events);
    QWidgetAction *waction1= new QWidgetAction(trayIconMenu);
    waction1->setDefaultWidget(batt_test2);
    trayIconMenu->addAction(waction1);

    trayIconMenu->addSeparator();

    WidgetBacklight* test_bl = new WidgetBacklight(this);
    test_bl->setup(0, client, events);

    QWidgetAction *waction0= new QWidgetAction(trayIconMenu);
    waction0->setDefaultWidget(test_bl);
    //trayIconMenu->insertAction(menuline,waction0);
    trayIconMenu->addAction(waction0);




    trayIcon->setIcon(QIcon(":/images/backlight.png"));

    trayIcon->show();

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::backlightChanged(int backlight, int value)
{
    qDebug()<<"blc: "<<backlight<<" "<<value<<"%";
}

void MainWindow::batteryCapacityChanged(int batt, PWRBatteryStatus stat)
{;
    qDebug()<<"batt cap "<<batt<<" cap:"<<stat.batteryCapacity<<" "<<stat.batteryCritical;
}

void MainWindow::batteryStateChanged(int bat, PWRBatteryStatus stat)
{
    qDebug()<<"batt "<<bat<<" state changed to "<<stat.batteryState<<" "<<stat.batteryCritical;
}

void MainWindow::acLineStateChanged(bool onExternalPower)
{
    qDebug()<<"AC power: "<<onExternalPower;
}

void MainWindow::profileChanged(QString profileID)
{
    qDebug()<<"profile changed to "<<profileID;
}
