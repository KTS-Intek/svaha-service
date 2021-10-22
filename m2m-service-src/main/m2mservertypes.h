#ifndef M2MSERVERTYPESANDDEFINES_H
#define M2MSERVERTYPESANDDEFINES_H

#include <QHash>
#include <QStringList>

typedef QHash<QString,QString> QStringHash;
typedef QHash<QString, QStringHash> QStringHashHash;



struct SyncServerSett{
    QString workDir;
    quint8 syncMode;
    quint8 maxFileCount;

    quint32 maxSizeMacTable;//maximum Mac count, обмежує hConnectedMac2date

    quint32 maxCountSha1LocalFsParallel;//maximum thread for check sha1


    qint32 maxSizeSyncRequest; //maximum sync request list size
    quint32 maxCountSyncRequestParallel; //maximum request at the same time

    qint32 maxYearSave;//
    qint32 minUniqMacs;

    SyncServerSett() :
        syncMode(0), maxFileCount(0xFF),
        maxSizeMacTable(1000000),
        maxCountSha1LocalFsParallel(100000),
        maxSizeSyncRequest(100000), maxCountSyncRequestParallel(1),
        maxYearSave(1000),
        minUniqMacs(1000) {}
} ;



#endif // M2MSERVERTYPESANDDEFINES_H
