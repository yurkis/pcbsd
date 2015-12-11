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

/*!
\file
\brief Backlight related functions. This covers intel_backlight port controlled backlight
*/

#include "intel_backlight.h"
#include <QProcess>
#include <QStringList>
#include <QCoreApplication>
#include <QDebug>
#include <QFile>

///////////////////////////////////////////////////////////////////////////////
QStringList intel_backlight(QStringList args=QStringList())
{
    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start("/usr/local/bin/intel_backlight", args);
    while(proc.state()==QProcess::Starting || proc.state() == QProcess::Running){
         proc.waitForFinished(200);
         QCoreApplication::processEvents();
    }
    QString tmp = proc.readAllStandardOutput();
    if(tmp.endsWith("\n")){tmp.chop(1);} //remove the newline at the end

    return tmp.split("\n");
}

///////////////////////////////////////////////////////////////////////////////
int IBLBacklightLevel()
{
    QStringList out = intel_backlight();
    //Example:
    // current backlight value: 100% (4882/4882)
    if (out.size()<1) return 100;
    QStringList tmp = out[0].split(" ");
    if (tmp.size()<5)
        return 100;
    QString prc = tmp[3];
    if (!prc.endsWith("%"))
        return 100;
    prc=prc.left(prc.length() - 1);
    return prc.toInt();
}

///////////////////////////////////////////////////////////////////////////////
bool setIBLBacklightLevel(int percentage)
{
    if (percentage>100) percentage=100;
    if (percentage<0) percentage = 0;
    QStringList out = intel_backlight(QStringList()<<QString::number(percentage));
    return true;
}

///////////////////////////////////////////////////////////////////////////////
bool hasIntelBacklight()
{
    if (!QFile::exists("/usr/local/bin/intel_backlight"))
    {
        return false;
    }
    QStringList out = intel_backlight();
    if (out.size()<1) return false;
    return (out[0].startsWith("current backlight value:"));    
}
