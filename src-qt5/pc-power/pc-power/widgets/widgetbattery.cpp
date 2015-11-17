#include "widgetbattery.h"
#include "ui_widgetbattery.h"

#include <QDebug>
#include <QPixmap>
#include <QPainter>

static const char* const BASE_BATTERY_ICON = ":/images/battery.png";

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

    QPixmap batt_pixmap;
    batt_pixmap.load(BASE_BATTERY_ICON);
    QPainter painter(&batt_pixmap);
    int h= batt_pixmap.height();
    int w = batt_pixmap.width();

    QFont font = painter.font();
    font.setBold(true);
    font.setPixelSize(w/2);
    painter.setFont(font);

    painter.setPen(QPen(QColor(Qt::white)));
    painter.drawText(0,0,w,h,Qt::AlignCenter, QString::number(num));
    ui->battIcon->setPixmap(batt_pixmap);
}

void WidgetBattery::batteryChanged(int batt, PWRBatteryStatus stat)
{
    if (batt!=battNum) return;
    refreshUI(stat);
}

void WidgetBattery::refreshUI(PWRBatteryStatus stat)
{
    ui->capacity->setValue(stat.batteryCapacity);

    if (stat.batteryState == BATT_CHARGING)
    {
        ui->statusText->setText(tr("Charging..."));
    }
    else
    {
        QString text;
        if (stat.batteryTime>0)
        {
            text = QString(tr("Estimated time: %1:%2 ")).arg(stat.batteryTime/60,2,10,QChar('0')).arg(stat.batteryTime%60,2,10,QChar('0'));
        }
        ui->statusText->setText(text);


    }
}
