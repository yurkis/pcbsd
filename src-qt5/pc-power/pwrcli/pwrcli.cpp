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
PWRCLI::PWRCLI(QObject *parent) : QObject(parent)
{
    client = new QPWRDClient(this);
    pipeName = DEF_PWRD_PIPE_NAME;
}

///////////////////////////////////////////////////////////////////////////////
void PWRCLI::cmdHelp()
{
    qcout()<<"Power daemon clinet\n"<<"Usage:";
    qcout()<<"pwrd [-pipe] [-help] command\n";
    qcout()<<"   -pipe PIPENAME - set ull name of pwrd pipe\n";
    qcout()<<"   -help - display this message and exin\n";
    qcout()<<"Commands:\n";
    qcout()<<"  hwinfo - display haedware info (related to beacklight, battery, etc)\n";
    qcout()<<"  sb or stbrightness [NO] LEVEL - set brightness to LEVEL percents\n";
    qcout()<<"                         for backlight # NO. Brightness may be relative\n";
    qcout()<<"                         for example 'sb +25' or 'sb -10'\n";
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
        qCritical()<<"Unable connect to get hardware info";
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
            capAh = info.batteries[i].designCapacity / info.batteries[i].designVoltage * 1000;
            capLastAh = info.batteries[i].lastFullCapacity / info.batteries[i].designVoltage * 1000;
            health = capLastAh * 100 / capAh;
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

    client->setBacklightLevel(str, num);

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
        qcout()<<" On AC power : "<<ac.id<<"\t'"<<ac.name<<"'\n";
        qcout()<<" On battery  : "<<batt.id<<"\t'"<<batt.name<<"'\n";
        qcout()<<" On low power: "<<low_batt.id<<"\t'"<<low_batt.name<<"'\n";
    }
    else
    {
        qcout()<<"pwrd error: "<<client->lastPWRDError()<<"\n";
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

    emit finished();
}

