#ifndef SYSCTLUTILS_H
#define SYSCTLUTILS_H

#include <QString>

QString sysctl(QString sysctl);
long long sysctlAsInt(QString sysctl);
bool setSysctl(QString sysctl, QString value);
bool setSysctl(QString sysctl, long long value);

#endif // SYSCTLUTILS_H

