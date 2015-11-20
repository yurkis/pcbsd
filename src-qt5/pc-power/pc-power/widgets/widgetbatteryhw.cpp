#include "widgetbatteryhw.h"
#include "ui_widgetbatteryhw.h"

WidgetBatteryHW::WidgetBatteryHW(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetBatteryHW)
{
    ui->setupUi(this);
}

WidgetBatteryHW::~WidgetBatteryHW()
{
    delete ui;
}

bool WidgetBatteryHW::setup(int batt_no, QPWRDClient *client)
{
    if (!client) return false;
    PWRDHardwareInfo info;
    if (!client->getHardwareInfo(info)) return false;
    if (info.batteries.size()< batt_no) return false;

    PWRBatteryHardware hw = info.batteries[batt_no];
    ui->OEMInfoLabel->setText(hw.OEMInfo);
    ui->modelLabel->setText(hw.model);
    ui->serialLabel->setText(hw.serial);
    int capAh;
    capAh = (hw.designVoltage)?(int)((float)hw.designCapacity) / ((float)hw.designVoltage) * 1000.:0;
    ui->capacityLabel->setText(tr("%1 mAh").arg(QString::number(capAh)));
    ui->designVoltageLabel->setText(tr("%1 mV").arg(QString::number(hw.designVoltage)));
    ui->healthPB->setMaximum(hw.designCapacity);
    ui->healthPB->setValue(hw.lastFullCapacity);

    return true;
}
