/**************************************************************************
*   Copyright (C) 2015- by Yuri Momotyuk                                   *
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

#include "battery.h"
#include "pwrserver.h"
#include <QFile>
#include <QString>
#include <QTextStream>

#ifdef FAKE_BATT

const int batts = 1;

const char* const BATT_FILE="/usr/home/yurkis/batt.txt";

bool getBatteryHWInfo(int batt, PWRBatteryHardware& hwout)
{
    if (batt >= batts) return false;

    hwout.type="LIon";
    hwout.OEMInfo="ASUSTek";
    hwout.model=QString("X202-51") /*+ QString::number(batt)*/;
    hwout.designVoltage = 7400;
    hwout.serial="0000-0000-0000";
    hwout.designCapacity=38006;
    hwout.lastFullCapacity=29289;

    return true;
}

bool getBatteryStatus(int batt, PWRBatteryStatus& info)
{
    QFile file(BATT_FILE);
    if (!file.open(QFile::ReadOnly | QFile::Text)) return false;
    QTextStream s(&file);
    QString str;
    str = s.readLine();
    info.batteryCapacity = str.toInt();
    if (batt % 2)
        info.batteryCapacity = 100 - info.batteryCapacity;
    str = s.readLine();
    if (str.toLower().trimmed() == QString("c"))
        info.batteryState = BATT_CHARGING;
    else
        info.batteryState = BATT_DISCHARGING;
    str = s.readLine();
    info.batteryTime = str.toInt();
    str = s.readLine();
    info.powerConsumption = str.toInt();

    return true;

}

bool PwrServer::isOnACPower()
{
    QFile file(BATT_FILE);
    if (!file.open(QFile::ReadOnly | QFile::Text)) return false;
    QTextStream s(&file);
    QString str;
    str = s.readLine();
    str = s.readLine();
    return (str.toLower().trimmed() == QString("c"));
}
#endif
