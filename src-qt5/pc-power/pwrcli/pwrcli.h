#ifndef PWRCLI_H
#define PWRCLI_H

#include <QObject>
#include <QString>

#include "QPWRDClient.h"

class PWRCLI : public QObject
{
    Q_OBJECT
public:
    explicit PWRCLI(QObject *parent = 0);

private:
    QPWRDClient* client;
    QString      pipeName;

    void cmdHelp();
    void cmdHWInfo();

signals:
    void finished();

public slots:
    void run();
};

#endif // PWRCLI_H
