#include "backend.h"

#include <QDir>
#include <QDebug>
#include <QProcess>

CEncryptedStorage::CEncryptedStorage(QString storagePath, QString mountPoint)
{

}


CPEFSBackend &CPEFSBackend::ref()
{
    static CPEFSBackend* pMe=0;
    if (!pMe)
        pMe= new CPEFSBackend;
    return *pMe;
}

bool CPEFSBackend::createNewStorage(QString storagePath, CEncryptedStorage *storageObject)
{
    storagePath= QDir::absolutePath(storagePath);
    if (QDir::exists(storagePath))
    {
        qDebug()<<"Can't create encrypted storage: path already exists"
        return false;
    }
    if (!QDir::mkpath(storagePath))
    {
        qDebug()<<"Can't create encrypted storage dir";
        return false;
    }

    QProcess pefs;
    //pefs.start("pefs", QStringList()<<"-f"<<"-Z");
}
