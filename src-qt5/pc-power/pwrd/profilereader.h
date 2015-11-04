#ifndef PROFILEREADER_H
#define PROFILEREADER_H

#include "pwrdtypes.h"
#include "../common/include/serialize.h"


static const char* const DEF_PROFILE_ID = "default";

typedef struct _PWRProfileReader: public JSONProfile
{
    _PWRProfileReader();
    bool read(QString file);
    bool write(QString file=QString());
protected:
    QString fileName;
}PWRProfileReader;

#endif // PROFILEREADER_H

