#ifndef PWRD_H
#define PWRD_H

#include <QString>
#include <QVector>
#include <QStringList>

typedef struct _PWRCurrentInfo
{
    bool         onACPower;         //< trueif PC on AC power
    unsigned int powerConsumption;  //< Current power consumption (in mW)
    unsigned int batteryTime;       //< Battery lifetime (in minutes)
    unsigned int backlightLevel;    //< Backlight brightness in percents (0..100)
    _PWRCurrentInfo()
    {
        onACPower = true;
        powerConsumption = 0;
        batteryTime = 0;
        backlightLevel = 0;
    }
}PWRSuppllyInfo;

typedef struct _PWRBatteryHardware
{
    bool         hasBattery;       //< True if PC has battery
    QString      OEMInfo;          //< OEM name
    QString      model;            //< Battery model
    QString      serial;           //< Battery serial number
    QString      type;       //< Type of battary (LIon in most cases)
    unsigned int designCapacity;   //< Design capacity (in mWh)
    unsigned int lastFullCapacity; //< Last full charrrged capacity (in mWh)
    unsigned int designVoltage;    //< Design voltage (in mV)
    _PWRBatteryHardware()
    {
        hasBattery = false;
        designCapacity = 0;
        lastFullCapacity = 0;
        designVoltage = 0;
    }
}PWRBatteryHardware;

typedef struct _PWRBacklightHardware
{
    bool hasBacklight;
    QVector<unsigned int> backlightLevels;
    _PWRBacklightHardware()
    {
        hasBacklight = false;
    }
}WRBacklightInfo;

typedef struct _PWRACPIInfo
{
    bool hasSleepButton;             //< True if PC has 'sleep' button
    bool hasLid;                     //< True for notebooks
    QStringList possibleACPIStates;  //< Possible sleep states for PC ("S3","S4","S5" for example)
    QString powerBtnSleepState;      //< Sleep state for power button (sleep state eg "S5" or "NONE")
    QString sleepBtnSleepState;      //< Sleep state for power button (sleep state or "NONE")
    QString lidSleepState;           //< Sleep state for notebook lid (sleep state or "NONE")
    QString suspendSleepState;       //< Sleep state for 'suspend' (sleep state or "NONE")
    QString standbySleepState;       //< Sleep state for 'standby' (sleep state or "NONE")

    _PWRACPIInfo(){
        hasSleepButton = false;
        hasLid = false;
    }
}PWRACPIInfo;


#endif // PWRD_H

