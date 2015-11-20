#ifndef PWRDDEBUG_H
#define PWRDDEBUG_H

#include<QDebug>

//#define DEBUG_ON

class TRACER
{
public:
    TRACER(QString _name)
    {
        name= _name;
        QString out;
        for(int i=0;i<ident;i++) out+=".";
        ident++;
        out+=QString("->") + name;
        qDebug()<<out;
    }
    ~TRACER()
    {
        QString out;
        for(int i=0;i<ident;i++) out+=".";
        ident--;
        out+=QString("<-") + name;
        qDebug()<<out;
    }
private:
    static int ident;
    QString name;
};

int TRACER::ident = 0;

#ifdef DEBUG_ON
    #define TRACED_FN TRACER(__FUNCTION__);
#else
    #define TRACED_FN
#endif

#endif // PWRDDEBUG_H

