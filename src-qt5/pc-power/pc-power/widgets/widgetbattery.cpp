#include "widgetbattery.h"
#include "ui_widgetbattery.h"

#include <QDebug>

WidgetBattery::WidgetBattery(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetBattery)
{
    ui->setupUi(this);

    /*if (client)
        connect(client)*/
}

WidgetBattery::~WidgetBattery()
{
    delete ui;
}

void WidgetBattery::setup(int num, QPWRDClient *cl, QPWRDEvents *ev)
{
    battNum =num;
    client = cl;
    events = ev;
    ui->battNoLabel->setText(QString::number(num));
    if (client)
    {
        QVector<PWRBatteryStatus> stats;
        if (client->getBatteriesState(stats))
        {
            if (num<stats.size())
            {
                refreshUI(stats[num]);
            }
        }
    }
    if (events)
    {
        connect(events, SIGNAL(batteryCapacityChanged(int, PWRBatteryStatus)), this, SLOT(batteryChanged(int, PWRBatteryStatus)));
        connect(events, SIGNAL(batteryStateChanged(int, PWRBatteryStatus)), this, SLOT(batteryChanged(int, PWRBatteryStatus)));
    }
}

void WidgetBattery::batteryChanged(int batt, PWRBatteryStatus stat)
{
    if (batt!=battNum) return;
    refreshUI(stat);
}

void WidgetBattery::refreshUI(PWRBatteryStatus stat)
{
    ui->capacity->setValue(stat.batteryCapacity);
    switch(stat.batteryState)
    {
        case BATT_CHARGING:
            ui->stateLabel->setText(tr("(Charging)"));
            break;
        case BATT_DISCHARGING:
            ui->stateLabel->setText(tr("(Discharging)"));
            break;
        default:
            ui->stateLabel->setText(tr("(State unknown)"));
            break;
    }
    ui->timeLabel->setVisible(stat.batteryTime>0);
    ui->timeTextLabel->setVisible(stat.batteryTime>0);
}
