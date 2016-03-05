#include "notification.h"
#include "ui_notification.h"

#include <QDesktopWidget>

Notification::Notification(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Notification)
{
    ui->setupUi(this);
    //setWindowFlags(Qt::FramelessWindowHint);
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint|Qt::WindowStaysOnTopHint);
    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), qApp->desktop()->availableGeometry()));
    setAttribute(Qt::WA_ShowWithoutActivating);
}

Notification::~Notification()
{
    delete ui;
}

void Notification::setup(QPWRDEvents *_ev)
{
    ev = _ev;

    connect(ev, SIGNAL(backlightChanged(int,int)), this, SLOT(backlightChanged(int,int)));
    connect(ev, SIGNAL(batteryStateChanged(int,PWRBatteryStatus)), this, SLOT(batteryStateChanged(int,PWRBatteryStatus)));
    connect(ev, SIGNAL(acLineStateChanged(bool)), this, SLOT(acLineStateChanged(bool)));
    connect(ev, SIGNAL(profileChanged(QString)), this, SLOT(profileChanged(QString)));
}

void Notification::backlightChanged(int backlight, int value)
{
    ui->mainStack->setCurrentIndex(1);
    ui->backlightPB->setValue(value);
    QTimer::singleShot(0, this, SLOT(showNormal()));
    //show();
    //this->focusInEvent();
    QTimer::singleShot(2000, this, SLOT(hide()));
    //showNormal();
}

void Notification::batteryStateChanged(int bat, PWRBatteryStatus stat)
{

}

void Notification::acLineStateChanged(bool onExternalPower)
{

}

void Notification::profileChanged(QString profileID)
{

}
