#include "mainwindow.h"
#include "ui_mainwindow.h"

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
