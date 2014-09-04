#ifndef BACKEND_H
#define BACKEND_H

#include <QString>

class CEncryptedStorage
{
public:
    CEncryptedStorage(QString storagePath = QString(), QString mountPoint=QString());

};

class CPEFSBackend
{
    CPEFSBackend(){;};

public:
    static CPEFSBackend& ref();

    bool createNewStorage(QString storagePath, QString pass ,CEncryptedStorage* storageObject);

};

static CPEFSBackend PEFS= CPEFSBackend::ref();

#endif // BACKEND_H
