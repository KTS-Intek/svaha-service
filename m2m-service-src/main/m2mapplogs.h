#ifndef M2MAPPLOGS_H
#define M2MAPPLOGS_H

///[!] sharedmemory
#include "src/shared/sharedmemowriterapplogbase.h"

//let it be, I will add something later


class M2MAppLogs : public SharedMemoWriterAppLogBase
{
    Q_OBJECT
public:
    explicit M2MAppLogs(const QString &sharedMemoName, const QString &semaName, const QString &write2fileName,
                        const int &delay, const int &delay2fileMsec, const bool &verboseMode, QObject *parent = nullptr);

signals:

};

#endif // M2MAPPLOGS_H
