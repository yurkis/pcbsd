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
