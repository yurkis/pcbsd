#include "widgetsleepbuttons.h"
#include "ui_widgetsleepbuttons.h"

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
    return true;
    client = cl;
    if (client) return false;

    ui->sleepButton->setVisible(false);
    ui->hibernateButton->setVisible(false);

    for(int i=0; i<possibleACPIStates.size(); i++)
    {
        if (possibleACPIStates[i].trimmed().toUpper() == "S3") ui->sleepButton->setVisible(true);
        else if (possibleACPIStates[i].trimmed().toUpper() == "S4") ui->hibernateButton->setVisible(true);
    }

    return ui->sleepButton->isVisible() || ui->hibernateButton->isVisible();
}

void WidgetSleepButtons::on_sleepButton_clicked()
{
    if (client) client->setACPIState("S3");
}

void WidgetSleepButtons::on_hibernateButton_clicked()
{
    if (client) client->setACPIState("S4");
}
