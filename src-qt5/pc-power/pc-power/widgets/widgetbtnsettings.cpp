#include "widgetbtnsettings.h"
#include "ui_widgetbtnsettings.h"

WidgetBtnSettings::WidgetBtnSettings(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetBtnSettings)
{
    ui->setupUi(this);
}

WidgetBtnSettings::~WidgetBtnSettings()
{
    delete ui;
}

void WidgetBtnSettings::setup(PWRHWInfo hwinfo,
                              QVector<SSleepStateDescription> descr,
                              QString pwrBtnState, QString sleepBtnState, QString lidState)
{
    hwInfo = hwinfo;
    ACPIdesr = descr;
    ui->powerBtnCB->clear();
    ui->sleepBtnCB->clear();
    ui->lidCB->clear();

    for (int i=0;i<descr.size();i++)
    {
        ui->powerBtnCB->addItem(descr[i].description);
        ui->sleepBtnCB->addItem(descr[i].description);
        ui->lidCB->addItem(descr[i].description);
    }
    bool isshow = hwInfo.hasSleepButton;
    ui->sleepBtnCB->setVisible(isshow);
    ui->sleepBtnDesc->setVisible(isshow);
    ui->sleepBtnImg->setVisible(isshow);


    isshow = hwInfo.hasLid;
    ui->lidCB->setVisible(isshow);
    ui->lidDesc->setVisible(isshow);
    ui->lidImg->setVisible(isshow);

    setState(pwrBtnState, sleepBtnState, lidState);
}

void WidgetBtnSettings::setState(QString powerBtn, QString sleepBtn, QString lid)
{
    for(int i=0; i<ACPIdesr.size(); i++)
    {
        if (powerBtn == ACPIdesr[i].state)
        {
            ui->powerBtnCB->setCurrentIndex(i);
            break;
        }
    }//for sleep mode descriptions

    if (hwInfo.hasSleepButton)
    {
        for(int i=0; i<ACPIdesr.size(); i++)
        {
            if (sleepBtn == ACPIdesr[i].state)
            {
                ui->sleepBtnCB->setCurrentIndex(i);
                break;
            }
        }//for sleep mode descriptions
    }//if sleep button present
    if (hwInfo.hasLid)
    {
        for(int i=0; i<ACPIdesr.size(); i++)
        {
            if (lid == ACPIdesr[i].state)
            {
                ui->lidCB->setCurrentIndex(i);
                break;
            }
        }//for sleep mode descriptions
    }//if sleep button present
}

QString WidgetBtnSettings::powerBtnState()
{
    if (ui->powerBtnCB->currentIndex() < ACPIdesr.size())
        return ACPIdesr[ui->powerBtnCB->currentIndex()].state;
    else
        return "NONE";
}

QString WidgetBtnSettings::sleepBtnState()
{
    if (ui->sleepBtnCB->currentIndex() < ACPIdesr.size())
        return ACPIdesr[ui->sleepBtnCB->currentIndex()].state;
    else
        return "NONE";
}

QString WidgetBtnSettings::lidState()
{
    if (ui->lidCB->currentIndex() < ACPIdesr.size())
        return ACPIdesr[ui->lidCB->currentIndex()].state;
    else
        return "NONE";
}
