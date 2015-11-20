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

#include "pwrcli.h"

#include <QCoreApplication>
#include <QStringList>
#include <QTextStream>
#include <QDebug>

///////////////////////////////////////////////////////////////////////////////
inline QTextStream& qcout()
{
    static QTextStream ts(stdout);
    return ts;
}

///////////////////////////////////////////////////////////////////////////////
QString strAlign(const QString& str, int length)
{
    QString out = str;
    length = length - str.length();
    for(int i=0; i<length; i++, out+=' ');
    return out;
}

///////////////////////////////////////////////////////////////////////////////
PWRCLI::PWRCLI(QObject *parent) : QObject(parent)
{
    client = new QPWRDClient(this);
    pipeName = DEF_PWRD_PIPE_NAME;
}

///////////////////////////////////////////////////////////////////////////////
void PWRCLI::cmdHelp()
{
    qcout()<<"Power daemon client\n"<<"Usage:\n";
    qcout()<<"pwrd [-pipe] [-help] command\n";
    qcout()<<"   -pipe PIPENAME - set full name of pwrd pipe\n";
    qcout()<<"   -help - display this message and exit\n";
    qcout()<<"Commands:\n";
    qcout()<<"  status - show current status (brightness level, battery status, etc\n";
    qcout()<<"  sb or setbrightness [NO] LEVEL - set brightness to LEVEL percents\n";
    qcout()<<"                         for backlight # NO. Brightness may be relative\n";
    qcout()<<"                         for example 'sb +25' or 'sb -10'\n";
    qcout()<<"  sleep - sleep system\n";
    qcout()<<"  hibernate - hibernate system\n";
    qcout()<<"  hwinfo - display hardware info (related to beacklight, battery, etc)\n";
    qcout()<<"  ap or activeprofiles - show active profiles\n";
    qcout()<<"  lp or listprofiles - show all profiles\n";
    qcout()<<"  profile [NAME] - display profile NAME info. If name is blank\n";
    qcout()<<"                         display current profile info\n";
    qcout()<<"  currprofile - display current profile name\n";
}

///////////////////////////////////////////////////////////////////////////////
void PWRCLI::cmdHWInfo()
{
    if (!client->connect(pipeName))
    {
        qCritical()<<"Unable connect to pwrd";
        return;
    }
    PWRDHardwareInfo info;
    if (!client->getHardwareInfo(info))
    {
        qcout()<<"pwrd error: "<<client->lastPWRDError()<<"\n";
        return;
    }
    qcout()<<"Basic ACPI: "<<"\n";
    qcout()<<"\t"<<"Baklights:"<<info.basic.numBacklights<<"\n";
    qcout()<<"\t"<<"Batteries:"<<info.basic.numBatteries<<"\n";    
    if (info.basic.possibleACPIStates.size())
    {
        qcout()<<"\t"<<"Supported ACPI power states:"<<"\n";
        for (int i=0; i<info.basic.possibleACPIStates.size(); i++)
        {
            QString desc;
            if (info.basic.possibleACPIStates[i] == "S3") desc="(sleep)";
            else if (info.basic.possibleACPIStates[i] == "S4") desc="(hibernate)";
            else if (info.basic.possibleACPIStates[i] == "S5") desc="(power off)";
            qcout()<<"\t\t"<<info.basic.possibleACPIStates[i]<<" "<<desc<<"\n";
        }
    }

    for (int i=0; i<info.batteries.size(); i++)
    {
        int capAh=0,  capLastAh=0, health=0;
        if(info.batteries[i].designVoltage)
        {
            capAh = (info.batteries[i].designVoltage)?(int)((float)info.batteries[i].designCapacity) / ((float)info.batteries[i].designVoltage) * 1000. : 0;
            capLastAh = (info.batteries[i].designVoltage)?(int)((float)info.batteries[i].lastFullCapacity) / ((float)info.batteries[i].designVoltage) * 1000. : 0;
            health = (capAh)?(int)((float)capLastAh) * 100. / capAh:100;
        }

        qcout()<<QString("Battery %1:").arg(i)<<"\n";
        qcout()<<"\t"<<"OEM             :"<<info.batteries[i].OEMInfo<<"\n";
        qcout()<<"\t"<<"Model           :"<<info.batteries[i].model<<"\n";
        qcout()<<"\t"<<"Type            :"<<info.batteries[i].type<<"\n";
        qcout()<<"\t"<<"Serial          :"<<info.batteries[i].serial<<"\n";
        qcout()<<"\t"<<"Design capacity :"<<info.batteries[i].designCapacity<<" mWh ";
        if (capAh) qcout()<<"("<<capAh<<" mAh)";
        qcout()<<"\n";
        qcout()<<"\t"<<"Last capacity   :"<<info.batteries[i].lastFullCapacity<<" mWh ";
        if (capLastAh)
        {
            qcout()<<"("<<capLastAh<<" mAh) health - "<<health<<"%";
        }
        qcout()<<"\n";
        qcout()<<"\t"<<"Design volatage :"<<info.batteries[i].designVoltage<<"mV\n";
    }
}

///////////////////////////////////////////////////////////////////////////////
void PWRCLI::cmdSetBacklight(QStringList args)
{
    if (!args.size())
        return;
    int num = PWR_ALL;
    QString str = args[0];
    if (args.size()>1)
    {
        num = str.toInt();
        str = args[1];
    }

    if (!client->connect(pipeName))
    {
        qCritical()<<"Unable connect to pwrd";
        return;
    }

    if (!client->setBacklightLevel(str, num))
    {
        qcout()<<"pwrd error: "<<client->lastPWRDError()<<"\n";
    }

}

///////////////////////////////////////////////////////////////////////////////
void PWRCLI::cmdGetActiveProfiles()
{
    PWRProfileInfoBasic ac,batt,low_batt;

    if (!client->connect(pipeName))
    {
        qCritical()<<"Unable connect to pwrd";
        return;
    }

    if (client->getActiveProfiles(&ac, &batt, &low_batt))
    {
        qcout()<<"State        Profile id\t\tProfile description\n";
        qcout()<<"-------------------------------------------------------\n";
        qcout()<<" On AC power : "<<strAlign(ac.id,18)<<ac.name<<"'\n";
        qcout()<<" On battery  : "<<strAlign(batt.id,18)<<batt.name<<"'\n";
        qcout()<<" On low power: "<<strAlign(low_batt.id,18)<<low_batt.name<<"'\n";
    }
    else
    {
        qcout()<<"pwrd error: "<<client->lastPWRDError()<<"\n";
    }
}

///////////////////////////////////////////////////////////////////////////////
void PWRCLI::cmdListProfiles()
{
    if (!client->connect(pipeName))
    {
        qCritical()<<"Unable connect to pwrd";
        return;
    }

    QVector<PWRProfileInfoBasic> profiles;

    if (!client->getProfiles(profiles))
    {
        qcout()<<"pwrd error: "<<client->lastPWRDError()<<"\n";
    }

    qcout()<<"Profile is\t\tProfile description\n";
    qcout()<<"-------------------------------------------------------\n";

    for (int i=0; i<profiles.size(); i++)
    {
        qcout()<<strAlign(profiles[i].id,24)<<profiles[i].name<<"\n";
    }
}

///////////////////////////////////////////////////////////////////////////////
void PWRCLI::cmdShowProfile(QStringList args)
{
    if (!client->connect(pipeName))
    {
        qCritical()<<"Unable connect to pwrd";
        return;
    }

    PWRProfile profile;
    QString id;

    if (args.size()) id = args[0];

    if (!client->getProfile(id, profile))
    {
        qcout()<<"pwrd error: "<<client->lastPWRDError()<<"\n";
        return;
    }

    qcout()<<"Profile "<<profile.id<<"\n";
    qcout()<<" Description :"<<profile.description<<"\n";
    qcout()<<" LCD backlight level: "<<profile.lcdBrightness<<"%\n";
    qcout()<<" State of buttons:\n";
    qcout()<<"   Power button:"<<profile.btnPowerSate<<"\n";
    qcout()<<"   Sleep button:"<<profile.btnSleepSate<<"\n";
    qcout()<<"   List switch :"<<profile.lidSwitchSate<<"\n";
}

void PWRCLI::cmdGetBacklightLevels()
{
    if (!client->connect(pipeName))
    {
        qCritical()<<"Unable connect to pwrd";
        return;
    }

    QVector<int> levels;
    if (!client->getAllBacklighsLevel(levels))
    {
        qcout()<<"pwrd error: "<<client->lastPWRDError()<<"\n";
        return;
    }

    for (int i=0; i<levels.size(); i++)
    {
        qcout()<<"Backlight #"<<i<<" brightness: "<<levels[i]<<"%\n";
    }

}

///////////////////////////////////////////////////////////////////////////////
void PWRCLI::cmdGetCurrentProfile()
{
    if (!client->connect(pipeName))
    {
        qCritical()<<"Unable connect to pwrd";
        return;
    }

    PWRProfileInfoBasic info;

    if (!client->getCurrentProfileID(info))
    {
        qcout()<<"pwrd error: "<<client->lastPWRDError()<<"\n";
        return;
    }

    qcout()<<"Current profile:\n";
    qcout()<<"  ID: "<<info.id<<"\n";
    qcout()<<"  Description: "<<info.id<<"\n";
}

///////////////////////////////////////////////////////////////////////////////
void PWRCLI::cmdGetStatus()
{
    if (!client->connect(pipeName))
    {
        qCritical()<<"Unable connect to pwrd";
        return;
    }

    bool isACLine = true;
    if (!client->getACLineState(isACLine))
    {
        qcout()<<"pwrd error: "<<client->lastPWRDError()<<"\n";
        return;
    }

    qcout()<<"Power state: ";
    if (isACLine)
    {
        qcout()<<"External power\n";
    }
    else
    {
        qcout()<<"Battery\n";
    }

    QVector<PWRBatteryStatus> batts;
    if (!client->getBatteriesState(batts))
    {
        qcout()<<"pwrd error: "<<client->lastPWRDError()<<"\n";
        return;
    }

    for(int i=0; i<batts.size(); i++)
    {
        qcout()<<"Battery #"<<i<<"\n";
        qcout()<<"  Current state : ";
        switch (batts[i].batteryState)
        {
            case BATT_CHARGING:
                qcout()<<"Charhing";
                break;
            case BATT_DISCHARGING:
                qcout()<<"Discharging";
                break;
            default:
                qcout()<<"Unknown";
        }
        qcout()<<"\n";

        qcout()<<"  Current rate  : "<<batts[i].batteryCapacity<<"%\n";
        qcout()<<"  Remaining time: ";
        if (batts[i].batteryTime)
            qcout()<<batts[i].batteryTime/60<<":"<<batts[i].batteryTime%60<<"\n";
         else
            qcout()<<"Unknown\n";
        qcout()<<"  Power consumption:"<<batts[i].powerConsumption<<" (mW)\n";
    }


}

///////////////////////////////////////////////////////////////////////////////
void PWRCLI::cmdSleep()
{
    if (!client->connect(pipeName))
    {
        qCritical()<<"Unable connect to pwrd";
        return;
    }

    PWRDHardwareInfo info;

    //Check if sleep state supported
    if (!client->getHardwareInfo(info))
    {
        qcout()<<"pwrd error: "<<client->lastPWRDError()<<"\n";
        return;
    }

    if (!info.basic.possibleACPIStates.contains("S3"))
    {
        qcout()<<"Sleep state (S3) is not supported\n";
        return;
    }
    if (!client->setACPIState("S3"))
    {
        qcout()<<"pwrd error: "<<client->lastPWRDError()<<"\n";
        return;
    }
}

///////////////////////////////////////////////////////////////////////////////
void PWRCLI::cmdHibernate()
{
    if (!client->connect(pipeName))
    {
        qCritical()<<"Unable connect to pwrd";
        return;
    }

    PWRDHardwareInfo info;

    //Check if sleep state supported
    if (!client->getHardwareInfo(info))
    {
        qcout()<<"pwrd error: "<<client->lastPWRDError()<<"\n";
        return;
    }

    if (!info.basic.possibleACPIStates.contains("S4"))
    {
        qcout()<<"Hibernate state (S4) is not supported\n";
        return;
    }
    if (!client->setACPIState("S4"))
    {
        qcout()<<"pwrd error: "<<client->lastPWRDError()<<"\n";
        return;
    }
}

///////////////////////////////////////////////////////////////////////////////
void PWRCLI::run()
{
    QStringList args = QCoreApplication::arguments();
    if (args.size()<2)
    {
        cmdHelp();
        emit finished();
        return;
    }
    int arg=1;
    QString arg1 = args[arg];

    if (arg1 == "-pipe")
    {
        if (args.size()<4)
        {
            cmdHelp();
            emit finished();
            return;
        }
        pipeName = args[2];
        arg1 = args[3];
        arg=3;
    }

    if (arg1 == "-help")
    {
        cmdHelp();
        emit finished();
    }
    else if(arg1 == "hwinfo")
    {
        cmdHWInfo();
        emit finished();
    }
    else if((arg1 == "setbrightness") || (arg1 == "sb"))
    {
        cmdSetBacklight(args.mid(arg+1));
        emit finished();
    }
    else if((arg1 == "activeprofiles") || (arg1 == "ap"))
    {
        cmdGetActiveProfiles();
        emit finished();
    }
    else if((arg1 == "listprofiles") || (arg1 == "lp"))
    {
        cmdListProfiles();
        emit finished();
    }
    else if((arg1 == "profile") )
    {
        cmdShowProfile(args.mid(arg+1));
        emit finished();
    }
    else if((arg1 == "backlight") || (arg1 == "b"))
    {
        cmdGetBacklightLevels();
        emit finished();
    }
    else if((arg1 == "currprofile") || (arg1 == "cp"))
    {
        cmdGetCurrentProfile();
        emit finished();
    }
    else if((arg1 == "status") || (arg1 == "s"))
    {
        cmdGetStatus();
        emit finished();
    }
    else if((arg1 == "sleep") )
    {
        cmdSleep();
        emit finished();
    }
    else if((arg1 == "hibernate") )
    {
        cmdHibernate();
        emit finished();
    }
    emit finished();
}

