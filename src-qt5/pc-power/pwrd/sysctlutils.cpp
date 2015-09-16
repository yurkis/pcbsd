#include "sysctlutils.h"

#include <sys/types.h>
#include <sys/sysctl.h>

#include <QDebug>

QString sysctl(QString sysctl)
{
   char result[1000];
   size_t len = sizeof(result);
   sysctlbyname(sysctl.toLocal8Bit(), result, &len, NULL, 0);
   result[len] = '\0';
   return QString(result);
}

long long sysctlAsInt(QString sysctl)
{
   long long result = 0;
   size_t len = sizeof(result);
   sysctlbyname(sysctl.toLocal8Bit(), &result, &len, NULL, 0);
   return result;
}

bool setSysctl(QString sysctlName, QString value)
{
    int mib[64];
    size_t len = sysctlName.split(".").size();
    if (len>=64)
        return false;

    if (sysctlnametomib(sysctlName.toLocal8Bit(), mib, &len) < 0)
    {
        return false;
    }
    if ( sysctl(mib, len, NULL, 0, value.toLocal8Bit(), value.length()) < 0 )
    {
        return false;
    }
    return true;
}

bool setSysctl(QString sysctlName, long long value)
{
    int mib[64];
    size_t len = sysctlName.split(".").size();
    if (len>=64)
        return false;

    if (sysctlnametomib(sysctlName.toLocal8Bit(), mib, &len) < 0)
    {
        return false;
    }
    if ( sysctl(mib, len, NULL, 0, &value, sizeof(value)) < 0 )
    {
        return false;
    }
    return true;
}
