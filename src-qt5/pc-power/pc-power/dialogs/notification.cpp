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

    setParent(0); // Create TopLevel-Widget
    //setAttribute(Qt::WA_NoSystemBackground, true);
    //setAttribute(Qt::WA_TranslucentBackground, true);

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
    if (last_low != stat.batteryCritical)
    {
        last_low = stat.batteryCritical;
        if (last_low)
        {
            notify(eCN_BATT_CRITICAL, 2);
        }//if low battery
    }//if batteryCritical state changed
}

void Notification::acLineStateChanged(bool onExternalPower)
{
    QString img;
    if (onExternalPower) img = QString(":/images/ac_power.png");
    else img = QString(":/images/batt_power.png");

    ui->ACStateImage->setPixmap(QPixmap(img));

    PWRProfileInfoBasic currp;
    if (cl)
    {
        cl->getCurrentProfileID(currp);
    }
    ui->profileNameLabel->setText(currp.name);

    if (notify(eCN_AC, 0))
    {
        QString msg = (onExternalPower)?tr("AC adaptor plugged in"):tr("AC adaptor unplugged");
        ui->reasonLabel->setText(msg);
    }
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

    if (notify(eCN_PROFILE, 0))
    {
        ui->reasonLabel->setText(tr("Profile changed"));
    }
}

void Notification::hideMe()
{
    currNotification = eCN_NONE;
    hide();
}

bool Notification::notify(Notification::ECurrentNotification level, int page_no)
{
    if (level>currNotification)
        return false;
    currNotification = level;

    ui->mainStack->setCurrentIndex(page_no);

    QTimer::singleShot(0, this, SLOT(showNormal()));

    timer->stop();
    timer->start(1000);
    return true;
}
