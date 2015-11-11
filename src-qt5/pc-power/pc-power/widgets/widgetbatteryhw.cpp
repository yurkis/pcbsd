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
