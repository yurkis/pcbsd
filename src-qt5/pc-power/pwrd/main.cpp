#include <QCoreApplication>

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include "pwrserver.h"

#include "battery.h"
#include "sysctlutils.h"
#include "profilereader.h"
#include "intel_backlight.h"

const char* const LOCK_FILE = "/var/run/pc-pwr";

static PwrServer *s = NULL;

static void globalSignalHandler(int sig)
{
    if (!s) return;
    s->signalHandler(sig);
}

static void init_daemon()
{
    int i,lfp;
    char str[10];
    if(getppid()==1)
        return; /* already a daemon */
    i=fork();
    if (i<0)
        exit(1); /* fork error */
    if (i>0)
        exit(0); /* parent exits */

    /* child (daemon) continues */
    setsid(); /* obtain a new process group */

    for (i=getdtablesize();i>=0;--i)
        close(i); /* close all descriptors */
    i=open("/dev/null",O_RDWR); dup(i); dup(i); /* handle standart I/O */

    umask(027); /* set newly created file permissions */

    //chdir(RUNNING_DIR); /* change running directory */
    lfp=open(LOCK_FILE,O_RDWR|O_CREAT,0640);
    if (lfp<0)
        exit(1); /* can not open */
    if (lockf(lfp,F_TLOCK,0)<0)
        exit(0); /* can not lock */
    /* first instance continues */
    sprintf(str,"%d\n",getpid());
    write(lfp,str,strlen(str)); /* record pid to lockfile */
    signal(SIGCHLD,SIG_IGN); /* ignore child */
    signal(SIGTSTP,SIG_IGN); /* ignore tty signals */
    signal(SIGTTOU,SIG_IGN);
    signal(SIGTTIN,SIG_IGN);
    signal(SIGHUP,globalSignalHandler); /* catch hangup signal */
    signal(SIGTERM,globalSignalHandler); /* catch kill signal */
}

void debug()
{
    /*PWRBatteryHardware hw;
    PWRSuppllyInfo info;

    getBatteryHWInfo(0, hw, info);
    qDebug()<<hw.hasBattery;
    qDebug()<<info.powerConsumption;
    qDebug()<<hw.model;
    qDebug()<<hw.OEMInfo;

    qDebug()<<sysctl("hw.acpi.suspend_state");
    setSysctl("security.jail.allow_raw_sockets", 0);
    qDebug()<<sysctlAsInt("security.jail.allow_raw_sockets");
    */
    //PWRProfileReader rdr;
    //rdr.read("/home/yurkis//projects/my-pcbsd/src-qt5/pc-power/pwrd/conf/profiles/pc-fullpower.profile");
    //qDebug()<<rdr.id;
    //qDebug()<<rdr.lcdBrightness;
    setIBLBacklightLevel(30);
    qDebug()<<IBLBacklightLevel();


}

int main(int argc, char *argv[])
{
    bool isForeground = true;
    QStringList serverArgs;
    QCoreApplication a(argc, argv);


    for(int i=1; i<argc; i++)
    {

        if (QString(argv[i]) == QString("-d"))
        {
            isForeground = false;
            continue;
        }

        serverArgs<<QString(argv[i]);
    }

    if (!isForeground)
        init_daemon();

    s = new PwrServer(&a);

    debug();

    if (s->start(serverArgs))
    {
        return a.exec();
    }
    else
    {
        return 1;
    }
}

