#include "widgetbacklight.h"
#include "ui_widgetbacklight.h"

#include <QDebug>

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

void WidgetBacklight::setup(int num, QPWRDClient *cl, QPWRDEvents *ev)
{
    blNum =num;
    client = cl;
    events = ev;
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

    ignoreEvents = false;

    if (events)
    {
        connect(events, SIGNAL(backlightChanged(int,int)), this, SLOT(pwrdValueChanged(int,int)));
    }
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
