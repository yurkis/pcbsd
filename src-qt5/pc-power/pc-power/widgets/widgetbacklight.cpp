
#include <widgets/widgetbacklight.h>
#include "widgetbacklight.h"
#include "ui_widgetbacklight.h"

#include <QDebug>

WidgetBacklight::WidgetBacklight(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetBacklight)
{
    client=NULL;
    events=NULL;
    ui->setupUi(this);
}

WidgetBacklight::~WidgetBacklight()
{
    delete ui;
}

void WidgetBacklight::setup(int num, QPWRDClient *cl, QPWRDEvents *ev, int value)
{
    blNum =num;
    client = cl;
    events = ev;

    if (value<0) setCurrValue(num);
    else ui->level->setValue(value);

    ignoreEvents = false;

    if (events)
    {
        connect(events, SIGNAL(backlightChanged(int,int)), this, SLOT(pwrdValueChanged(int,int)));
    }
}

void WidgetBacklight::setCurrValue(int num)
{
    if (client)
    {
        QVector<int> vals;
        if (client->getAllBacklighsLevel(vals))
        {
            if (num<vals.size())
            {
                refreshUI(vals[num]);
            }
        }
    }
}

int WidgetBacklight::value()
{
    return ui->level->value();
}

void WidgetBacklight::setValue(int val)
{
    ui->level->setValue(val);
}

void WidgetBacklight::pwrdValueChanged(int backlight, int value)
{
    if (backlight != blNum) return;
    if (!ignoreEvents) refreshUI(value);
}

void WidgetBacklight::refreshUI(int value)
{
    ui->level->setValue(value);
}

void WidgetBacklight::on_level_sliderMoved(int position)
{
    if (!client) return;
    ignoreEvents= true;
    qDebug()<<position;
    client->setBacklightLevel(position, blNum);
}

void WidgetBacklight::on_level_valueChanged(int value)
{
    Q_UNUSED(value)
    if (!client) return;

    //client->setBacklightLevel(value, blNum);
    //ignoreEvents= false;
}

void WidgetBacklight::on_level_sliderReleased()
{
    ignoreEvents= false;
}
