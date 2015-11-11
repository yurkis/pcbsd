#include "widgetbacklight.h"
#include "ui_widgetbacklight.h"

WidgetBacklight::WidgetBacklight(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetBacklight)
{
    ui->setupUi(this);
}

WidgetBacklight::~WidgetBacklight()
{
    delete ui;
}
