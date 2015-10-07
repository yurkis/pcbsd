#include "intel_backlight.h"
#include <QProcess>
#include <QStringList>
#include <QCoreApplication>
#include <QDebug>
#include <QFile>

QStringList intel_backlight(QStringList args=QStringList())
{
    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start("/usr/local/bin/intel_backlight", args);
    while(proc.state()==QProcess::Starting || proc.state() == QProcess::Running){
         proc.waitForFinished(200);
         QCoreApplication::processEvents();
    }
    QString tmp = proc.readAllStandardOutput();
    if(tmp.endsWith("\n")){tmp.chop(1);} //remove the newline at the end

    return tmp.split("\n");
}

int IBLBacklightLevel()
{
    QStringList out = intel_backlight();
    //Example:
    // current backlight value: 100% (4882/4882)
    if (out.size()<1) return 100;
    QStringList tmp = out[0].split(" ");
    if (tmp.size()<5)
        return 100;
    QString prc = tmp[3];
    if (!prc.endsWith("%"))
        return 100;
    prc=prc.left(prc.length() - 1);
    return prc.toInt();
}

bool setIBLBacklightLevel(int percentage)
{
    if (percentage>100) percentage=100;
    if (percentage<0) percentage = 0;
    QStringList out = intel_backlight(QStringList()<<QString::number(percentage));
    return true;
}

bool hasIntelBacklight()
{
    return QFile::exists("/usr/local/bin/intel_backlight");
}
