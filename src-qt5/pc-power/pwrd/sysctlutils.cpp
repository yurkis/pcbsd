#include "sysctlutils.h"

#include <sys/types.h>
#include <sys/sysctl.h>

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
