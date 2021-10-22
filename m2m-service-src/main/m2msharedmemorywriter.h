#ifndef M2MSHAREDMEMORYWRITER_H
#define M2MSHAREDMEMORYWRITER_H

///[!] sharedmemory
#include "src/shared/sharedmemowriter.h"


#include "m2mservertypes.h"


class M2MSharedMemoryWriter : public SharedMemoWriter
{
    Q_OBJECT
public:
    explicit M2MSharedMemoryWriter(const QString &sharedMemoName, const QString &semaName, const bool &verboseMode,QObject *parent = nullptr);

    static QString strFromStrHash(const QStringHash &h);

    QVariantHash fromConnectionTable(QStringHash hashMacRemoteId, QStringHash hashMacDevId, QStringHash hashTime, QStringHashHash hashAboutObject);


signals:
    void sendConnectionTable();

public slots:
    void onThreadStarted();

    void onConnectionTableChanged();

    void onConnectionTableData(QStringHash hashMacRemoteId, QStringHash hashMacDevId, QStringHash hashTime, QStringHashHash hashAboutObject);

};

#endif // M2MSHAREDMEMORYWRITER_H
