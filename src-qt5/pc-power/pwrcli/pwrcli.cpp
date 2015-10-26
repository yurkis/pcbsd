#include "pwrcli.h"

#include <QCoreApplication>
#include <QStringList>
#include <QTextStream>
#include <QDebug>

inline QTextStream& qcout()
{
    static QTextStream ts(stdout);
    return ts;
}

PWRCLI::PWRCLI(QObject *parent) : QObject(parent)
{
    client = new QPWRDClient(this);
    pipeName = DEF_PWRD_PIPE_NAME;
}

void PWRCLI::cmdHelp()
{
    qcout()<<"Power daemon clinet\n"<<"Usage:";
}

void PWRCLI::cmdHWInfo()
{
    qDebug()<<pipeName;
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
        int capAh,  capLastAh, health;
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

void PWRCLI::cmdSetBacklight(QStringList args)
{
    qDebug()<<args;
    if (!args.size())
        return;
    int num = 0;
    int brightness;
    QString str = args[0];
    if (args.size()>1)
    {
        num = str.toInt();
        str = args[1];
    }
    QString sign="";
    if (str.startsWith("+") || str.startsWith("-"))
    {
        sign = str[0];
        str=str.right(str.length()-1);
    }
    qDebug()<<str<<sign;

}

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

    qDebug()<<arg1;
    qDebug()<<args.size();

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
    else if(arg1 == "setbacklight")
    {
        cmdSetBacklight(args.mid(arg+1));
        emit finished();
    }

    if (!client->connect(pipeName))
    {
        qCritical()<<"Unable connect to pwrd";
        return;
    }
    //client->getBacklightLevel();

    emit finished();
}

