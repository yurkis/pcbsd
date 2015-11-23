#ifndef CONNECTERRORDIALOG_H
#define CONNECTERRORDIALOG_H

#include <QDialog>

#include "QPWRDClient.h"
#include "QPWRDEvents.h"

namespace Ui {
class ConnectErrorDialog;
}

class ConnectErrorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConnectErrorDialog(QWidget *parent = 0);
    ~ConnectErrorDialog();

    void execute(QPWRDClient* cl, QPWRDEvents* ev);

private slots:
    void on_closeAppButton_clicked();

    void on_tryagainButton_clicked();

private:
    Ui::ConnectErrorDialog *ui;
    QPWRDClient* client;
    QPWRDEvents* events;
};

#endif // CONNECTERRORDIALOG_H
