#include "connecterrordialog.h"
#include "ui_connecterrordialog.h"

ConnectErrorDialog::ConnectErrorDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConnectErrorDialog)
{
    ui->setupUi(this);
}

ConnectErrorDialog::~ConnectErrorDialog()
{
    delete ui;
}

void ConnectErrorDialog::execute(QPWRDClient *cl, QPWRDEvents *ev)
{
    if ((!cl) || (!ev)) return;
    client = cl;
    events = ev;
    exec();
}

void ConnectErrorDialog::on_closeAppButton_clicked()
{
    exit(1);
}

void ConnectErrorDialog::on_tryagainButton_clicked()
{
    if (client->connect() && events->connect())
    {
        setResult(QDialog::Accepted);
        close();
    }
}
