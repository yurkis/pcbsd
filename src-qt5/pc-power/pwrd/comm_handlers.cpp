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
/*!
\file
\brief JSON control pipe command handlers (part of PwrServer class)
*/

#include "pwrserver.h"
#include "serialize.h"
#include "../common/include/protocol.h"

#include "hw/backlight.h"
#include "hw/battery.h"
#include "hw/buttons.h"
#include "hw/intel_backlight.h"
#include "hw/sleep.h"

#include <QString>
#include <QJsonObject>
#include <QJsonDocument>

///////////////////////////////////////////////////////////////////////////////
static QJsonObject RESULT_SUCCESS()
{
    QJsonObject res;
    res[MSG_RESULT] = MSG_RESULT_SUCCESS;
    return res;
}

///////////////////////////////////////////////////////////////////////////////
static QJsonObject RESULT_FAIL(QString reason = QString("Unknown reason"))
{
    QJsonObject res;
    res[MSG_RESULT] = MSG_RESULT_FAIL;
    res[MSG_RESULT_FAIL_REASON] = reason;
    return res;
}

///////////////////////////////////////////////////////////////////////////////
QJsonObject PwrServer::parseCommand(QString line)
{
    QJsonDocument jsonResponse = QJsonDocument::fromJson(line.toUtf8());
    QJsonObject root = jsonResponse.object();
    QJsonObject resp = RESULT_FAIL("Bad request");
    try{
      if (root.find(MSGTYPE_COMMAND) != root.end())
      {
          qDebug()<<"COMMAND:"<<root[MSGTYPE_COMMAND];

          if (root[MSGTYPE_COMMAND] == COMMAND_HWINFO)
          {
            resp = oncmdGetHWInfo();
          }
          else if (root[MSGTYPE_COMMAND] == COMMAND_GET_BACKLIGHT)
          {
              resp = oncmdGetBacklight();
          }
          else if (root[MSGTYPE_COMMAND] == COMMAND_SET_BACKLIGHT)
          {
              resp = oncmdSetBacklight(root);
          }
          else if (root[MSGTYPE_COMMAND] == COMMAND_ACTIVE_PROFILES)
          {
              resp = oncmdGetActiveProfiles();
          }
          else if (root[MSGTYPE_COMMAND] == COMMAND_GET_PROFILES)
          {
              resp = oncmdGetProfiles();
          }
          else if (root[MSGTYPE_COMMAND] == COMMAND_GET_PROFILE)
          {
              resp = oncmdGetProfile(root);
          }
          else if (root[MSGTYPE_COMMAND] == COMMAND_GET_CURRENT_PROFILE)
          {
              resp = oncmdGetCurrentProfile();
          }
          else if (root[MSGTYPE_COMMAND] == COMMAND_AC_STATUS)
          {
              resp = oncmdGetAcStatus();
          }
          else if (root[MSGTYPE_COMMAND] == COMMAND_GET_BATT_STATE)
          {
              resp = oncmdGetBattState();
          }
          else if (root[MSGTYPE_COMMAND] == COMMAND_SET_ACPI_STATE)
          {
              resp = oncmdSetACPIState(root);
          }
          else if (root[MSGTYPE_COMMAND] == COMMAND_APPLY_PROFILE)
          {
              resp = oncmdApplyProfile(root);
          }
          else if (root[MSGTYPE_COMMAND] == COMMAND_GET_BUTTONS_STATE)
          {
              resp = oncmdGetButtonsState();
          }
          else if (root[MSGTYPE_COMMAND] == COMMAND_SET_BUTTONS_STATE)
          {
              resp = oncmdSetButtonsState(root);
          }
          else if (root[MSGTYPE_COMMAND] == COMMAND_GET_SETTINGS)
          {
              resp = oncmdGetSettings();
          }
          else if (root[MSGTYPE_COMMAND] == COMMAND_SET_SETTINGS)
          {
              resp = oncmdSetSettings(root);
          }
          else if (root[MSGTYPE_COMMAND] == COMMAND_UPDATE_PROFILE)
          {
              resp = oncmdUpdateProfile(root);
          }
          else if (root[MSGTYPE_COMMAND] == COMMAND_REMOVE_PROFILE)
          {
              resp = oncmdRemoveProfile(root);
          }
      }
    }catch(...){
        resp = RESULT_FAIL("Internal error");
    }

    //qDebug()<<line;
    //sendResponse(resp, connections[sender].stream);
    return resp;
}

//////////////////////////////////////////////////////////////////////////////
QJsonObject PwrServer::oncmdGetHWInfo()
{
    QJsonObject resp = RESULT_SUCCESS();

    if (!settings.usingIntel_backlight)
    {
        resp[hwInfo.myname()] = hwInfo.toJSON();
        QVector2JSON(JSONBacklightHardware().myname(), backlightHW, resp);
    }
    else
    {
        //if using intel_backlight transmit fake hardware info
        JSONHWInfo fakeinfo = hwInfo;
        fakeinfo.numBacklights = 1;
        resp[fakeinfo.myname()] = fakeinfo.toJSON();

        QVector<JSONBacklightHardware> fakevec;
        JSONBacklightHardware fake;
        fakevec.push_back(fake);
        QVector2JSON(JSONBacklightHardware().myname(), fakevec, resp);
    }
    QVector2JSON(JSONBatteryHardware().myname(), battHW, resp);

    return resp;
}

///////////////////////////////////////////////////////////////////////////////
QJsonObject PwrServer::oncmdGetBacklight()
{
    QJsonObject resp = RESULT_SUCCESS();
    QJsonArray backlights;

    checkBacklights();

    for(int i=0; i<currBacklightLevels.size(); i++)
    {
        backlights.append((int)currBacklightLevels[i]);
    }

    /*
    if (settings.usingIntel_backlight)
    {
        backlights.append(IBLBacklightLevel());
    }
    else
    {
        for (int i=0; i<backlightHW.size(); i++)
        {
            backlights.append(backlightLevel(i));
        }
    }*/
    resp[BACKLIGHT_LEVELS] = backlights;
    return resp;
}

///////////////////////////////////////////////////////////////////////////////
QJsonObject PwrServer::oncmdSetBacklight(QJsonObject req)
{
    QJsonObject resp = RESULT_SUCCESS();

    int no = PWR_ALL;
    bool is_relative=false;
    int val;
    int start, end;
    if (req.find(BACKLIGHT_VALUE)==req.end())
    {
        return RESULT_FAIL("Bad request");
    }
    QString val_s = req[BACKLIGHT_VALUE].toString();
    val_s = val_s.trimmed();
    is_relative = (val_s.startsWith("-") || val_s.startsWith("+"));
    val = val_s.toInt();    

    if (req.find(BACKLIGHT_NUMBER)==req.end())
    {
        no = PWR_ALL;
    }
    else
    {
        no = req[BACKLIGHT_NUMBER].toInt();
        if (no != PWR_ALL)
        {
            if (!settings.usingIntel_backlight)
            {
                if ((no<0) || (no>=backlightHW.size()))
                    return RESULT_FAIL("Bad backlight number");
            }
            else
            {
                if (no != 0) return RESULT_FAIL("Bad backlight number");
            }
        }
    }

    if (no == PWR_ALL)
    {
        start = 0; end = backlightHW.size();
    }
    else
    {
        start = no; end= no+1;
    }

    if (settings.usingIntel_backlight)
    {
        //only one backlight using with Intel_backlight port
        start =0; end=1;
    }
    for(int i=start; i<end; i++)
    {
        if (!is_relative)
        {
            if (val<0) val = 5;
            if (val>100) val = 100;

            if (!settings.usingIntel_backlight)
            {
                setBacklightLevel(i, val);                
            }
            else
            {
                setIBLBacklightLevel(val);
            }
        }// if direct value
        else
        {
            int curr = (!settings.usingIntel_backlight)?backlightLevel(i):IBLBacklightLevel();
            val=val+curr;
            if (val<0) val = 0;
            if (val>100) val = 100;
            (!settings.usingIntel_backlight)?setBacklightLevel(i, val):setIBLBacklightLevel(val);
        }
    }
    checkBacklights();

    return resp;

}

///////////////////////////////////////////////////////////////////////////////
QJsonObject PwrServer::oncmdGetActiveProfiles()
{
    QJsonObject resp = RESULT_SUCCESS();
    resp[ON_AC_POWER_PROFILE_ID] = settings.onACProfile;
    resp[ON_AC_POWER_PROFILE_NAME] = findProfile(settings.onACProfile).description;
    resp[ON_BATTERY_PROFILE_ID] = settings.onBatteryProfile;
    resp[ON_BATTERY_PROFILE_NAME] = findProfile(settings.onBatteryProfile).description;
    resp[ON_LOW_BATTERY_PROFILE_ID] = settings.onLowBatteryProfile;
    resp[ON_LOW_BATTERY_PROFILE_NAME] = findProfile(settings.onLowBatteryProfile).description;
    return resp;
}

///////////////////////////////////////////////////////////////////////////////
QJsonObject PwrServer::oncmdGetProfiles()
{
    QJsonObject resp = RESULT_SUCCESS();
    QJsonArray arr;
    for (auto pi = profiles.begin(); pi != profiles.end(); ++pi)
    {
        QJsonObject item;
        item[PROFILE_ID] = pi->id;
        item[PROFILE_NAME] = pi->description;
        arr.append(item);
    }
    resp[PROFILES_ARRAY] = arr;

    return resp;
}

///////////////////////////////////////////////////////////////////////////////
QJsonObject PwrServer::oncmdGetProfile(QJsonObject req)
{
    QJsonObject resp = RESULT_SUCCESS();
    PWRProfileReader profile;
    if (req.contains(PROFILE_ID))
    {
        QString id = req[PROFILE_ID].toString();
        if (!profiles.contains(id))
        {
            return RESULT_FAIL("Profile not found");
        }
        profile = profiles[id];
    }
    else
    {
        profile = currProfile;
    }
    profile.toJSON(resp);
    return resp;
}

///////////////////////////////////////////////////////////////////////////////
QJsonObject PwrServer::oncmdGetCurrentProfile()
{
    QJsonObject resp = RESULT_SUCCESS();
    resp[PROFILE_ID] = currProfile.id;
    resp[PROFILE_NAME] = currProfile.description;
    return resp;
}

///////////////////////////////////////////////////////////////////////////////
QJsonObject PwrServer::oncmdGetAcStatus()
{
    QJsonObject resp = RESULT_SUCCESS();
    resp[AC_POWER] = isOnACPower();
    return resp;
}

///////////////////////////////////////////////////////////////////////////////
QJsonObject PwrServer::oncmdGetBattState()
{
    QJsonObject resp = RESULT_SUCCESS();
    QJsonArray arr;
    for(int i=0; i<battHW.size(); i++)
    {
        JSONBatteryStatus item;
        if (getBatteryStatus(i, item))
        {
            item.batteryCritical = (int)item.batteryCapacity<=settings.lowBatteryCapacity;
            arr.append(item.toJSON());
        }
    }
    resp[JSONBatteryStatus().myname()] = arr;

    return resp;
}

///////////////////////////////////////////////////////////////////////////////
QJsonObject PwrServer::oncmdSetACPIState(QJsonObject req)
{
    if (!req.contains(ACPI_STATE))
    {
        return RESULT_FAIL("Bad request");
    }
    QString state = req[ACPI_STATE].toString();

    state=state.trimmed().toUpper();
    if (state.length()!=2)
    {
        return RESULT_FAIL("Bad request aparameter");
    }

    if (state == "S5")
    {
        return RESULT_FAIL("Power off is forbidden");
    }

    if (!hwInfo.possibleACPIStates.contains(state))
    {
        return RESULT_FAIL("Sleep state is not supported");
    }

    if (!ACPISleep(state))
    {
        return RESULT_FAIL("Could not change state");
    }

    return RESULT_SUCCESS();
}

///////////////////////////////////////////////////////////////////////////////
QJsonObject PwrServer::oncmdApplyProfile(QJsonObject req)
{
    if (!req.contains(PROFILE_ID))
    {
        return RESULT_FAIL("Bad request");
    }

    if (!profiles.contains(req[PROFILE_ID].toString()))
    {
        return RESULT_FAIL("Profile not found");
    }
    applyProfile(req[PROFILE_ID].toString());

    return RESULT_SUCCESS();
}

///////////////////////////////////////////////////////////////////////////////
QJsonObject PwrServer::oncmdGetButtonsState()
{
    QJsonObject resp = RESULT_SUCCESS();

    checkButtons();

    resp[BTN_POWER_STATE]= currPowerBtnState;
    resp[BTN_SLEEP_STATE]= currSleepBtnState;
    resp[LID_SWITCH_SATE]= currLidSwitchState;

    return resp;
}

///////////////////////////////////////////////////////////////////////////////
QJsonObject PwrServer::oncmdSetButtonsState(QJsonObject req)
{
    QString state;
    if ((hwInfo.hasSleepButton) && (req.contains(BTN_SLEEP_STATE)))
    {
        state = req[BTN_SLEEP_STATE].toString().trimmed().toUpper();
        if ((hwInfo.possibleACPIStates.contains(state)) || (state=="NONE"))
        {
            setSleepBtnSleepState(state);
        }
    }
    if ((hwInfo.hasLid) && (req.contains(LID_SWITCH_SATE)))
    {
        state = req[LID_SWITCH_SATE].toString().trimmed().toUpper();
        if ((hwInfo.possibleACPIStates.contains(state)) || (state=="NONE"))
        {
            setLidSleepState(state);
        }
    }
    if (req.contains(BTN_POWER_STATE))
    {
        state = req[BTN_POWER_STATE].toString().trimmed().toUpper();
        if ((hwInfo.possibleACPIStates.contains(state)) || (state=="NONE"))
        {
            setPowerBtnSleepState(state);
        }
    }

    checkButtons();    

    return RESULT_SUCCESS();
}

///////////////////////////////////////////////////////////////////////////////
QJsonObject PwrServer::oncmdGetSettings()
{
    QJsonObject resp = RESULT_SUCCESS();

    settings.toJSON(resp);

    return resp;
}

///////////////////////////////////////////////////////////////////////////////
QJsonObject PwrServer::oncmdSetSettings(QJsonObject req)
{
    if (!settings.allowSettingsChange)
        return RESULT_FAIL("Not allowed");

    JSONDaemonSettings s;
    if (!s.fromJSON(req))
    {
        RESULT_FAIL("Bad request");
    }

    settings.lowBatteryCapacity = s.lowBatteryCapacity;
    settings.onACProfile = s.onACProfile;
    settings.onBatteryProfile = s.onBatteryProfile;
    settings.onLowBatteryProfile = s.onLowBatteryProfile;
    settings.save(confFile);

    checkState(true);

    return RESULT_SUCCESS();
}

///////////////////////////////////////////////////////////////////////////////
QJsonObject PwrServer::oncmdUpdateProfile(QJsonObject req)
{
    if (!settings.allowProfileChange)
        return RESULT_FAIL("Not allowed");

    if (!req.contains(JSONProfile().myname()))
        return RESULT_FAIL("Bad request");

    PWRProfileReader profile;

    if (!profile.fromJSON(req))
    {
        return RESULT_FAIL("Bad request");
    }

    profile.write(settings.profilesPath+QString("/")+profile.id+QString(".profile"));

    profiles[profile.id]=profile;

    emitEvent(EVENT_PROFILES_UPDATED, QJsonObject());

    checkState(true);

    return RESULT_SUCCESS();
}

///////////////////////////////////////////////////////////////////////////////
QJsonObject PwrServer::oncmdRemoveProfile(QJsonObject req)
{
    return RESULT_SUCCESS();
}

