#include "buttons.h"
#include "sysctlutils.h"

static const char* const SLEEP_SYSCTL = "hw.acpi.sleep_button_state";
static const char* const POWER_SYSCTL = "hw.acpi.power_button_state";
static const char* const LID_SYSCTL = "hw.acpi.lid_switch_state";

QString sleepBtnSleepState()
{
    return sysctl(SLEEP_SYSCTL);
}


QString powerBtnSleepState()
{
    return sysctl(POWER_SYSCTL);
}

QString lidSleepState()
{
    return sysctl(LID_SYSCTL);
}

bool setSleepBtnSleepState(QString state)
{
    return setSysctl(SLEEP_SYSCTL, state);
}

bool setPowerBtnSleepState(QString state)
{
    return setSysctl(POWER_SYSCTL, state);
}

bool setLidSleepState(QString state)
{
    return setSysctl(LID_SYSCTL, state);
}
