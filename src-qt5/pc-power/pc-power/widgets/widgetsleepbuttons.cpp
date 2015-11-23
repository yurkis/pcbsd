#include "widgetsleepbuttons.h"
#include "ui_widgetsleepbuttons.h"

#include <QDebug>

WidgetSleepButtons::WidgetSleepButtons(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetSleepButtons)
{
    ui->setupUi(this);
}

WidgetSleepButtons::~WidgetSleepButtons()
{
    delete ui;
}

bool WidgetSleepButtons::setup(QPWRDClient *cl, QStringList possibleACPIStates)
{
    client = cl;
    if (!client) return false;

    bool isSleep = false;
    bool isHibernate = false;

    for(int i=0; i<possibleACPIStates.size(); i++)
    {        
        if (possibleACPIStates[i].trimmed().toUpper() == "S3") isSleep= true;
        else if (possibleACPIStates[i].trimmed().toUpper() == "S4") isHibernate=true;
    }

    ui->sleepButton->setVisible(isSleep);
    ui->hibernateButton->setVisible(isHibernate);

    return isSleep || isHibernate;
}

void WidgetSleepButtons::on_sleepButton_clicked()
{
    if (client) client->setACPIState("S3");
}

void WidgetSleepButtons::on_hibernateButton_clicked()
{
    if (client) client->setACPIState("S4");
}
