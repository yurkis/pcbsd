/**************************************************************************
*   Copyright (C) 2015 by Yuri Momotyuk                                   *
*   yurkis@pcbsd.org                                                      *
*                                                                         *
*   Permission is hereby granted, free of charge, to any person obtaining *
*   a copy of this software and associated documentation files (the       *
*   "Software"), to deal in the Software without restriction, including   *
*   without limitation the rights to use, copy, modify, merge, publish,   *
*   distribute, sublicense, and/or sell copies of the Software, and to    *
*   permit persons to whom the Software is furnished to do so, subject to *
*   the following conditions:                                             *
*                                                                         *
*   The above copyright notice and this permission notice shall be        *
*   included in all copies or substantial portions of the Software.       *
*                                                                         *
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       *
*   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    *
*   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*
*   IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR     *
*   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, *
*   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR *
*   OTHER DEALINGS IN THE SOFTWARE.                                       *
***************************************************************************/

#include "sysctlutils.h"

#include <sys/types.h>
#include <sys/sysctl.h>

#include <QDebug>

////////////////////// from libPCBSD  ///////////////////////////////////////
QString sysctl(QString sysctl)
{
   char result[1000];
   size_t len = sizeof(result);
   sysctlbyname(sysctl.toLocal8Bit(), result, &len, NULL, 0);
   result[len] = '\0';
   return QString(result);
}

/////////////////////// from libPCBSD /////////////////////////////////////////
long long sysctlAsInt(QString sysctl)
{
   long long result = 0;
   size_t len = sizeof(result);
   sysctlbyname(sysctl.toLocal8Bit(), &result, &len, NULL, 0);
   return result;
}

///////////////////////////////////////////////////////////////////////////////
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

///////////////////////////////////////////////////////////////////////////////
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

///////////////////////////////////////////////////////////////////////////////
bool sysctlPresent(QString sysctlName)
{
    int mib[64];
    size_t len = sysctlName.split(".").size();
    if (len>=64)
        return false;

    if (sysctlnametomib(sysctlName.toLocal8Bit(), mib, &len) < 0)
    {
        return false;
    }
    return true;
}
