#include "notification.h"
#include "ui_notification.h"

#include <QDesktopWidget>
#include <QPixmap>
#include <QDebug>

Notification::Notification(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Notification)
{
    ui->setupUi(this);
    //setWindowFlags(Qt::FramelessWindowHint);
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint|Qt::WindowStaysOnTopHint);
    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), qApp->desktop()->availableGeometry()));
    setAttribute(Qt::WA_ShowWithoutActivating);

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(hideMe()));
    currNotification = eCN_NONE;
}

Notification::~Notification()
{
    delete ui;
}

void Notification::setup(QPWRDEvents *_ev, QPWRDClient *_cl)
{
    ev = _ev;
    cl = _cl;

    connect(ev, SIGNAL(backlightChanged(int,int)), this, SLOT(backlightChanged(int,int)));
    connect(ev, SIGNAL(batteryStateChanged(int,PWRBatteryStatus)), this, SLOT(batteryStateChanged(int,PWRBatteryStatus)));
    connect(ev, SIGNAL(acLineStateChanged(bool)), this, SLOT(acLineStateChanged(bool)));
    connect(ev, SIGNAL(profileChanged(QString)), this, SLOT(profileChanged(QString)));
}

void Notification::backlightChanged(int backlight, int value)
{
    ui->backlightPB->setValue(value);
    notify(eCN_BACKLIGHT, 1);
}

void Notification::batteryStateChanged(int bat, PWRBatteryStatus stat)
{
    static bool last_low = stat.batteryCritical;
    //if
}

void Notification::acLineStateChanged(bool onExternalPower)
{
    QString img;
    if (onExternalPower) img = QString(":/images/ac_power.png");
    else img = QString(":/images/batt_power.png");

    ui->ACStateImage->setPixmap(QPixmap(img));
    notify(eCN_AC, 0);
}

void Notification::profileChanged(QString profileID)
{
    bool isOnAC = true;
    PWRProfileInfoBasic currp;
    currp.name = profileID;
    if (cl)
    {
        cl->getACLineState(isOnAC);
        cl->getCurrentProfileID(currp);
    }

    ui->profileNameLabel->setText(currp.name);

    notify(eCN_PROFILE, 0);
}

void Notification::hideMe()
{
    currNotification = eCN_NONE;
    hide();
}

void Notification::notify(Notification::ECurrentNotification level, int page_no)
{
    if (level>currNotification)
        return;
    currNotification = level;

    ui->mainStack->setCurrentIndex(page_no);

    QTimer::singleShot(0, this, SLOT(showNormal()));

    timer->stop();
    timer->start(1000);
}
