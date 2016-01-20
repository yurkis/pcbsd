#ifndef PROFILEEDITDIALOG_H
#define PROFILEEDITDIALOG_H

#include <QDialog>
#include <QVector>

#include <QPWRDClient.h>

#include "ssdescription.h"

namespace Ui {
class ProfileEditDialog;
}

class ProfileEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProfileEditDialog(QWidget *parent = 0);
    void setup(QString id, QPWRDClient* client, QVector<SSleepStateDescription> descr);
    void setup(QPWRDClient* client, QVector<SSleepStateDescription> descr);
    ~ProfileEditDialog();

private slots:
    void on_currValuesBtn_clicked();

    void on_saveBtn_clicked();

private:
    Ui::ProfileEditDialog *ui;

    QPWRDClient* client;
    QVector<SSleepStateDescription> ACPIdesr;

    void setFromProfile(QString id);
    void setCurrentValues();
};

#endif // PROFILEEDITDIALOG_H
