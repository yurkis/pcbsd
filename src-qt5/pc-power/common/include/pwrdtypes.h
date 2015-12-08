/*!
\file
\brief PWRD related data types
*/

#ifndef PWRD_H
#define PWRD_H

#include <QString>
#include <QVector>
#include <QStringList>

//! Default PWRD control pipe name
static const char* const DEF_PWRD_PIPE_NAME = "/var/run/pwrd.pipe";
//! Default PWRD events pipe name
static const char* const DEF_PWRD_EVENTS_PIPE_NAME = "/var/run/pwrd.events.pipe";

//! Using instead of index if operation should affect all batteries or backlights
const int PWR_ALL = -1;

//! Battery states
typedef enum
{
    BATT_CHARGING = 0, ///< Battery is charging
    BATT_DISCHARGING,  ///< Battery is discharging
    BATT_STATE_UNKNOWN,///< Battery state is unknown
    BATT_STATE_MAX
}PWRBatteryState;

//! Current battery state
typedef struct _PWRBatteryState
{
    PWRBatteryState batteryState;   ///< Current battery state
    unsigned int batteryCapacity;   ///< Battery capacity in percents (0..100)
    unsigned int powerConsumption;  ///< Current power consumption (in mW)
    unsigned int batteryTime;       ///< Battery lifetime (in minutes)
    bool         batteryCritical;
    _PWRBatteryState()
    {
        batteryState = BATT_STATE_UNKNOWN;
        batteryCapacity = 0;
        batteryTime = 0;
        powerConsumption = 0;
        batteryCritical = false;
    }
}PWRBatteryStatus;

//! Battery hardware info
typedef struct _PWRBatteryHardware
{
    QString      OEMInfo;          ///< OEM name
    QString      model;            ///< Battery model
    QString      serial;           ///< Battery serial number
    QString      type;             ///< Type of battary (LIon in most cases)
    unsigned int designCapacity;   ///< Design capacity (in mWh)
    unsigned int lastFullCapacity; ///< Last full charrrged capacity (in mWh)
    unsigned int designVoltage;    ///< Design voltage (in mV)
    _PWRBatteryHardware()
    {        
        designCapacity = 0;
        lastFullCapacity = 0;
        designVoltage = 0;
    }
}PWRBatteryHardware;

//! Backlight hardware info
typedef struct _PWRBacklightHardware
{
    QVector<unsigned int> backlightLevels;
    _PWRBacklightHardware()
    {

    }
}PWRBacklightHardware;

//! Basic hardware info
typedef struct _PWRHWInfo
{
    int numBatteries;                ///< Number of supported batteries
    int numBacklights;               ///< Number of supported backlights
    bool hasSleepButton;             ///< True if PC has 'sleep' button
    bool hasLid;                     ///< True for notebooks (has lid)
    QStringList possibleACPIStates;  ///< Possible sleep states for PC ("S3","S4","S5" for example)

    _PWRHWInfo()
    {
        numBatteries = 0;
        numBacklights = 0;
        hasSleepButton = false;
        hasLid = false;
    }
}PWRHWInfo;

//! Power profile
typedef struct _PWRProfile
{
    QString id;                   ///< Profile ID
    QString description;          ///< Profile description
    QString btnPowerSate;         ///< Sleep state for 'Power' button
    QString btnSleepSate;         ///< Sleep state for 'Sleep' button
    QString lidSwitchSate;        ///< Sleep state for lid
    unsigned int lcdBrightness;   ///< LCD brightness level
    _PWRProfile()
    {
        lcdBrightness = 75;
    }
}PWRProfile;

//! PWRD daemon settings
typedef struct _PWRDaemonSettings
{
    int     lowBatteryCapacity;  ///< Battery capacity to enable 'Low battery' profile
    QString onBatteryProfile;    ///< Profile that should be applied when device is on battery power
    QString onACProfile;         ///< Profile that should be applied when device is on external power
    QString onLowBatteryProfile; ///< Profile that should be applied when capacity of battery is low (lower or eqal than lowBatteryCapacity)
    bool usingIntel_backlight;   ///< True if using intel_backlight port for changing backlight level
    bool allowSettingsChange;    ///< True if remote settings change is allowed. Can't be changed using API
    bool allowProfileChange;     ///< True if remote profiles change is allowed. Can't be changed using API
    _PWRDaemonSettings()
    {
        lowBatteryCapacity=0;
        usingIntel_backlight = false;
        allowSettingsChange = false;
        allowProfileChange = false;
    }
}PWRDaemonSettings;

#endif // PWRD_H

