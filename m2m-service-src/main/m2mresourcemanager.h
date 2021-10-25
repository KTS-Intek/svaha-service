#ifndef M2MRESOURCEMANAGER_H
#define M2MRESOURCEMANAGER_H

#include <QObject>

///[!] svaha-service
#include "m2m-service-src/connection-holder/m2mconnholderserver.h"
#include "m2m-service-src/backup/m2mbackupmanager.h"
#include "m2m-service-src/main/m2msharedmemorywriter.h"
#include "m2m-service-src/main/m2mlocalsocket.h"



//it creates all necessary objects


class M2MResourceManager : public QObject
{
    Q_OBJECT
public:
    explicit M2MResourceManager(QObject *parent = nullptr);

    void startEverything(const bool &verboseMode);

signals:

    //to all
    void killAllObjects();

    void goGoGo();

    void addEvent2log(QString message);


//    //from local socket to m2mserver
//    void killClientNow(QString id, bool byDevId);




//    //from m2m server to shared memory
//    void onConnectionTableChanged();

//    void onConnectionTableData(QStringHash hashMacRemoteId, QStringHash hashMacDevId, QStringHash hashTime, QStringHashHash hashAboutObject);

//    //from shared memory to m2m-server
//    void sendConnectionTable();



    //settings to M2M server
    void setTimeouts(int zombieMsec, int msecAlive, int timeOutGMsec, int timeOutBMsec);

    void setBackupWorkDirectory(QString workDir);

    void setDataConnectionParams(QString serverDataIP, quint16 minDataPort, quint16 maxDataPort);

    void setServicePortSmart(quint16 port);

    void setAllowAndBlockList(QStringList allowIpList, QStringList blockThisIp);

    //settings to backupmanager
    void setSyncParams(QString workDir, quint8 syncMode, quint8 maxFileCount, quint32 maxSizeMacTable, quint32 maxCountSha1LocalFsParallel, qint32 maxSizeSyncRequest, quint32 maxCountSyncRequestParallel, qint32 maxYearSave, qint32 minUniqMacs);


public slots:
    void reloadSettings();

    void sendGlobalSettings(const int &zombieMsec, const int &msecAlive);


    void killApp();

    void onTmrCreate();

    //from M2MServer

    void onFailed2startServer(QString message);

    void addEvent2logSlot(QString message);


private:
    M2MLocalSocket *createLocalSocket(const bool &verboseMode);

    M2MSharedMemoryWriter *createSharedMemoryWriter(const bool &verboseMode);

    M2MConnHolderServer *createM2MServer(const bool &verboseMode, M2MSharedMemoryWriter *writer, M2MLocalSocket *extSocket);

    M2MBackupManager *createBackupManager(const bool &verboseMode, M2MConnHolderServer *server);



    struct ManagerParams
    {
        bool allowSharedMemory;
        bool activeDbgMessages;
        bool verboseMode;

        quint16 m2mServiceFailsConter;

        ManagerParams() : allowSharedMemory(true), activeDbgMessages(false), verboseMode(false),
            m2mServiceFailsConter(0) {}
    } myParams;
};

#endif // M2MRESOURCEMANAGER_H
