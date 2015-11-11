#include "widgetbattery.h"
#include "ui_widgetbattery.h"

WidgetBattery::WidgetBattery(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetBattery)
{
    ui->setupUi(this);
}

WidgetBattery::~WidgetBattery()
{
    delete ui;
}
