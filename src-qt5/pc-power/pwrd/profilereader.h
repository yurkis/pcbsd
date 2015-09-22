#ifndef PROFILEREADER_H
#define PROFILEREADER_H

#include "pwrdtypes.h"

typedef struct _PWRProfileReader:public PWRProfile
{
    _PWRProfileReader();
    bool read(QString file);
    bool write(QString file=QString());
protected:
    QString fileName;
}PWRProfileReader;

#endif // PROFILEREADER_H

