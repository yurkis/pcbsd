#ifndef PWRD_H
#define PWRD_H

#include <QString>
#include <QVector>
#include <QStringList>

static const char* const DEF_PWRD_PIPE_NAME = "/var/run/pwrd.pipe";

typedef enum
{
    BATT_CHARGING = 0,
    BATT_DISCHARGING,
    BATT_CRITICAL,
    BATT_STATE_UNKNOWN,
    BATT_STATE_MAX
}PWRBatteryState;

typedef struct _PWRSuppllyInfo
{    
    unsigned int batteryRate;       //< Battery rate in percents (0..100)
    PWRBatteryState batteryState;   //< Current battery state
    unsigned int powerConsumption;  //< Current power consumption (in mW)
    unsigned int batteryTime;       //< Battery lifetime (in minutes)
    unsigned int backlightLevel;    //< Backlight brightness in percents (0..100)
    _PWRSuppllyInfo()
    {
        powerConsumption = 0;
        batteryTime = 0;
        backlightLevel = 0;
    }
}PWRSuppllyInfo;

typedef struct _PWRBatteryHardware
{
    QString      OEMInfo;          //< OEM name
    QString      model;            //< Battery model
    QString      serial;           //< Battery serial number
    QString      type;       //< Type of battary (LIon in most cases)
    unsigned int designCapacity;   //< Design capacity (in mWh)
    unsigned int lastFullCapacity; //< Last full charrrged capacity (in mWh)
    unsigned int designVoltage;    //< Design voltage (in mV)
    _PWRBatteryHardware()
    {        
        designCapacity = 0;
        lastFullCapacity = 0;
        designVoltage = 0;
    }
}PWRBatteryHardware;

typedef struct _PWRBacklightHardware
{
    QVector<unsigned int> backlightLevels;
    _PWRBacklightHardware()
    {

    }
}PWRBacklightHardware;

typedef struct _PWRHWInfo
{
    int numBatteries;
    int numBacklights;
    bool hasSleepButton;             //< True if PC has 'sleep' button
    bool hasLid;                     //< True for notebooks
    QStringList possibleACPIStates;  //< Possible sleep states for PC ("S3","S4","S5" for example)

    _PWRHWInfo()
    {
        numBatteries = 0;
        numBacklights = 0;
        hasSleepButton = false;
        hasLid = false;
    }
}PWRHWInfo;


typedef struct _PWRProfile
{
    QString name;
    QString description;
    QString btnPowerSate;
    QString btnSleepSate;
    QString lidSwitchSate;
    unsigned int lcdBrightness;
    _PWRProfile()
    {
        lcdBrightness = 75;
    }
}PWRProfile;

#endif // PWRD_H

