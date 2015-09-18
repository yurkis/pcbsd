#ifndef SYSCTLUTILS_H
#define SYSCTLUTILS_H

#include <QString>

QString sysctl(QString sysctl);
long long sysctlAsInt(QString sysctl);
bool sysctlPresent(QString sysctlName);
bool setSysctl(QString sysctlName, QString value);
bool setSysctl(QString sysctlName, long long value);

#endif // SYSCTLUTILS_H

