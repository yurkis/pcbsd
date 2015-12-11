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

