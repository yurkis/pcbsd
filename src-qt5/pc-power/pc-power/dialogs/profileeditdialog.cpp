#include "profileeditdialog.h"
#include "ui_profileeditdialog.h"

ProfileEditDialog::ProfileEditDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProfileEditDialog)
{
    client=NULL;
    ui->setupUi(this);
}

void ProfileEditDialog::setup(QString id, QPWRDClient *cl, QVector<SSleepStateDescription> descr)
{
    ACPIdesr = descr;
    client= cl;
    if (!cl) return;
    setFromProfile(id);
}

void ProfileEditDialog::setup(QPWRDClient *cl, QVector<SSleepStateDescription> descr)
{
    ACPIdesr = descr;
    client = cl;
    setCurrentValues();
}

ProfileEditDialog::~ProfileEditDialog()
{
    delete ui;
}

void ProfileEditDialog::on_currValuesBtn_clicked()
{
    setCurrentValues();
}

void ProfileEditDialog::setFromProfile(QString id)
{
    if (!client)
        return;
    PWRProfile p;
    if (!client->getProfile(id, p))
            return;

    ui->backlight->setValue(p.lcdBrightness);
    ui->idLE->setText(p.id);
    ui->idLE->setEnabled(false);
    ui->descriptionLE->setText(p.description);

    PWRDHardwareInfo hwinfo;
    client->getHardwareInfo(hwinfo);

    QString pwr,sleep, lid;

    ui->buttonsWidget->setup(hwinfo.basic, ACPIdesr, p.btnPowerSate, p.btnSleepSate, p.lidSwitchSate);
}

void ProfileEditDialog::setCurrentValues()
{
    if (!client)
        return;
    int bl;
    if (client->getBacklightLevel(0,bl))
        ui->backlight->setValue(bl);

    PWRDHardwareInfo hwinfo;
    client->getHardwareInfo(hwinfo);

    QString pwr,sleep, lid;
    client->getButtonsState(pwr, sleep, lid);
    ui->buttonsWidget->setup(hwinfo.basic, ACPIdesr, pwr, sleep, lid);
}

void ProfileEditDialog::on_saveBtn_clicked()
{
    PWRProfile profile;
    profile.id = ui->idLE->text().replace(" ","_");
    profile.description = ui->descriptionLE->text();
    profile.lcdBrightness = ui->backlight->value();
    profile.btnPowerSate = ui->buttonsWidget->powerBtnState();
    profile.btnSleepSate = ui->buttonsWidget->sleepBtnState();
    profile.lidSwitchSate = ui->buttonsWidget->lidState();

    client->updateProfile(profile);

    accept();
}
