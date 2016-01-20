#ifndef WIDGETBTNSETTINGS_H
#define WIDGETBTNSETTINGS_H

#include <QWidget>

#include <QPWRDClient.h>

#include "ssdescription.h"

namespace Ui {
class WidgetBtnSettings;
}


class WidgetBtnSettings : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetBtnSettings(QWidget *parent = 0);
    ~WidgetBtnSettings();

    void setup(PWRHWInfo hwinfo,
               QVector<SSleepStateDescription> descr,
               QString pwrBtnState, QString sleepBtnState, QString lidState);
    void setState(QString powerBtn, QString sleepBtn, QString lid);
    QString powerBtnState();
    QString sleepBtnState();
    QString lidState();

private:
    Ui::WidgetBtnSettings *ui;

    PWRHWInfo hwInfo;
    QVector<SSleepStateDescription> ACPIdesr;
};

#endif // WIDGETBTNSETTINGS_H
