#include "matildasynchelper.h"
#include <QCryptographicHash>
//---------------------------------------------------------------------------------------------
MatildaSyncHelper::MatildaSyncHelper(QObject *parent) : QObject(parent)
{

}
//---------------------------------------------------------------------------------------------
QByteArray MatildaSyncHelper::calcSHA1_4File(const QString &fileName, bool &ok)
{
    //for files < 1MB - readAll, else readPart(1MB)/ MAX 10MB
    QFileInfo fi(fileName);
    qint64 fileSize = fi.size();

    ok = false;
    QFile file(fileName);
    QByteArray sha1;
    if(file.open(QFile::ReadOnly)){
        for(qint64 i = 0, readLen = 1000000, j = 0; j < 10 && i < fileSize; j++, i += readLen){
            QByteArray r = file.read(readLen);
            if(r.isEmpty())
                continue;
            sha1 = QCryptographicHash::hash(sha1 + r, QCryptographicHash::Sha1);

        }
        file.close();
        ok = !sha1.isEmpty();
    }
    return sha1;
}
//---------------------------------------------------------------------------------------------
