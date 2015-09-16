#include <QCoreApplication>

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include "pwrserver.h"

#include "acpiinfo.h"
#include "sysctlutils.h"

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

int main(int argc, char *argv[])
{
    bool isForeground = true;

    QCoreApplication a(argc, argv);


    for(int i=1; i<argc; i++)
    {
        if (QString(argv[i]) == QString("-d"))
        {
            isForeground = false;
        }
    }

    if (!isForeground)
        init_daemon();


    PWRBatteryHardware hw;
    PWRSuppllyInfo info;

    getBatteryInfo(0, hw, info);
    qDebug()<<hw.hasBattery;
    qDebug()<<info.powerConsumption;
    qDebug()<<hw.model;
    qDebug()<<hw.OEMInfo;

    qDebug()<<sysctl("hw.acpi.suspend_state");


    s = new PwrServer(&a);

    if (s->start())
    {
        return a.exec();
    }
    else
    {
        return 1;
    }
}

