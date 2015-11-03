#ifndef BUTTONS_H
#define BUTTONS_H

#include <QString>

QString sleepBtnSleepState();
QString powerBtnSleepState();
QString lidSleepState();

bool setSleepBtnSleepState(QString state);
bool setPowerBtnSleepState(QString state);
bool setLidSleepState(QString state);

#endif // BUTTONS_H

