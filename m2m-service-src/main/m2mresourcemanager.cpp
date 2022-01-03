#include "m2mresourcemanager.h"


///[!] matilda-bbb-m2m-server
#include "src/m2m-settings/settloader4m2mserver.h"


///[!] matilda-bbb-settings
#include "src/matilda/settloader4matilda.h"


M2MResourceManager::M2MResourceManager(QObject *parent) : QObject(parent)
{

}

//----------------------------------------------------------------------

void M2MResourceManager::startEverything(const bool &verboseMode)
{
    myParams.verboseMode = verboseMode;
    QTimer::singleShot(54, this, SLOT(onTmrCreate()));
}

//----------------------------------------------------------------------

void M2MResourceManager::reloadSettings()
{

    SettLoader4m2mServer sLoader;

    sLoader.checkDefSett(myParams.verboseMode);


    emit addEvent2log(QString("Reloading settings"));

    if(true){

        const quint16 minDataPort = sLoader.loadOneSett(SETT_SVAHA_DATA_START_PORT, myParams.verboseMode).toUInt();
        const quint16 maxDataPort = minDataPort + sLoader.loadOneSett(SETT_SVAHA_DATA_PORT_COUNT, myParams.verboseMode).toUInt();


        QString serverDataIP = sLoader.loadOneSett(SETT_MATILDA_CONF_IP, myParams.verboseMode).toString();
        if(serverDataIP.isEmpty())
            serverDataIP = "kts-m2m.ddns.net";


        emit setDataConnectionParams(serverDataIP, minDataPort, maxDataPort);
    }




//    server4matildaConf = sLoader.loadOneSett(SETT_MATILDA_CONF_IP).toString();
//       server4matildadev = sLoader.loadOneSett(SETT_MATILDA_DEV_IP).toString();

    if(true){



        const int zombieMsec = sLoader.loadOneSett(SETT_ZOMBIE_MSEC, myParams.verboseMode).toInt();// 15 * 60 * 1000 );
        const int killTmrMsec = sLoader.loadOneSett(SETT_TIME_2_LIVE, myParams.verboseMode).toInt();// 24 * 60 * 60 * 1000);

        sendGlobalSettings(zombieMsec, killTmrMsec);

        const quint16 m2mServiceConnHolderPort = quint16(sLoader.loadOneSett(SETT_SVAHA_SERVICE_PORT, myParams.verboseMode).toUInt());

        emit setServicePortSmart(m2mServiceConnHolderPort);
    }


    const QString workDir = sLoader.loadOneSett(SETT_SYNC_WORKDIR, myParams.verboseMode).toString();
    const quint8 syncMode = sLoader.loadOneSett(SETT_SYNC_MODE, myParams.verboseMode).toUInt();
    const quint8 maxFileCount = sLoader.loadOneSett(SETT_SYNC_MAX_FILE_COUNT, myParams.verboseMode).toUInt();

    const quint32 maxSizeMacTable = sLoader.loadOneSett(SETT_SYNC_MAX_SIZE_MAC_TABLE, myParams.verboseMode).toUInt();

    const quint32 maxCountSha1LocalFsParallel = sLoader.loadOneSett(SETT_SYNC_MAX_COUNT_SHA1_CHRSPRLL, myParams.verboseMode).toUInt();

    const qint32 maxSizeSyncRequest = sLoader.loadOneSett(SETT_SYNC_MAX_SIZE_SYNC_REQUEST, myParams.verboseMode).toInt();
    const qint32 maxCountSyncRequestParallel = sLoader.loadOneSett(SETT_SYNC_MAX_COUNT_SYNQ_RQSTPRLL, myParams.verboseMode).toUInt();

     const qint32 maxYearSave = sLoader.loadOneSett(SETT_SYNC_MAX_YEAR_SAVE, myParams.verboseMode).toInt();
     const qint32 minUniqMacs = sLoader.loadOneSett(SETT_SYNC_MIN_UNIQ_MAC_FILES, myParams.verboseMode).toInt();

     emit setSyncParams(workDir, syncMode, maxFileCount, maxSizeMacTable, maxCountSha1LocalFsParallel, maxSizeSyncRequest, maxCountSyncRequestParallel, maxYearSave, minUniqMacs);

     emit setBackupWorkDirectory(workDir);

//    //settings to M2M server



//    //settings to backupmanager
//    void setSyncParams(QString workDir, quint8 syncMode, quint8 maxFileCount, quint32 maxSizeMacTable, quint32 maxCountSha1LocalFsParallel, qint32 maxSizeSyncRequest, quint32 maxCountSyncRequestParallel, qint32 maxYearSave, qint32 minUniqMacs);

}

void M2MResourceManager::sendGlobalSettings(const int &zombieMsec, const int &msecAlive)
{

    SettLoader4matilda sLoader;
    if(myParams.verboseMode){
        qDebug() << "sendGlobalSettings "
                 << sLoader.loadOneSett(SETT_ALLOW_IP_LIST).toStringList()
                 << sLoader.loadOneSett(SETT_BLOCK_IP_LIST).toStringList();
    }

    emit setAllowAndBlockList(sLoader.loadOneSett(SETT_ALLOW_IP_LIST).toStringList(), sLoader.loadOneSett(SETT_BLOCK_IP_LIST).toStringList());



    emit setTimeouts(zombieMsec, msecAlive, sLoader.loadOneSett(SETT_TCP_READ_TO).toInt(), sLoader.loadOneSett(SETT_TCP_READ_TOB).toInt());

//    emit setNewTimeOutNow(sLoader.loadOneSett(SETT_TCP_READ_TO).toInt(), sLoader.loadOneSett(SETT_TCP_READ_TOB).toInt(), sLoader.loadOneSett(SETT_TCP_MNOZNYK).toLongLong());



          //    void setTimeouts(int zombieMsec, int timeOutGMsec, int timeOutBMsec);

}

//----------------------------------------------------------------------

void M2MResourceManager::killApp()
{
    emit killAllObjects();
    QThread::sleep(1);
    qApp->exit(APP_CODE_RESTART);
}

//----------------------------------------------------------------------

void M2MResourceManager::onTmrCreate()
{
    if(myParams.verboseMode)
        connect(this, &M2MResourceManager::addEvent2log, this, &M2MResourceManager::addEvent2logSlot);

    M2MLocalSocket *extSocket = createLocalSocket(myParams.verboseMode);

    createSharedMemory(myParams.verboseMode);

    auto *writer = createSharedMemoryWriter(myParams.verboseMode);
    auto *server = createM2MServer(myParams.verboseMode, writer, extSocket);
//    auto *manager =
    createBackupManager(myParams.verboseMode, server);




    QTimer::singleShot(111, this, SIGNAL(goGoGo()));


}

//----------------------------------------------------------------------

void M2MResourceManager::onFailed2startServer(QString message)
{
    myParams.m2mServiceFailsConter++;

    if(myParams.verboseMode)
        qDebug() << "M2MResourceManager " << myParams.m2mServiceFailsConter << message;

    if(myParams.m2mServiceFailsConter > 10)
        killApp();
}

//----------------------------------------------------------------------

void M2MResourceManager::addEvent2logSlot(QString message)
{
    qDebug() << "addEvent2log " << message;

}

//----------------------------------------------------------------------

M2MLocalSocket *M2MResourceManager::createLocalSocket(const bool &verboseMode)
{
    M2MLocalSocket *extSocket = new M2MLocalSocket(verboseMode);

    extSocket->activeDbgMessages = myParams.activeDbgMessages;
    extSocket->initializeSocket(MTD_EXT_NAME_SVAHA_SERVICE);

    QThread *extSocketThrd = new QThread;
    extSocketThrd->setObjectName("M2MLocalSocket");
    extSocket->moveToThread(extSocketThrd);


    connect(extSocket, SIGNAL(destroyed(QObject*)), extSocketThrd, SLOT(quit()));
    connect(extSocketThrd, SIGNAL(finished()), extSocketThrd, SLOT(deleteLater()));

#ifdef ENABLE_VERBOSE_SERVER
    connect(extSocket, &M2MLocalSocket::appendDbgExtData, this, &ZbyratorManager::appendDbgExtData );
#endif
    connect(extSocketThrd, &QThread::started, extSocket, &M2MLocalSocket::onThreadStarted);

    connect(extSocket, &M2MLocalSocket::reloadSett , this, &M2MResourceManager::reloadSettings  );

//    connect(extSocket, &M2MLocalSocket::onConfigChanged , this, &M2MResourceManager::reloadSettings  ); //WTF how it can be???

    connect(extSocket, &M2MLocalSocket::killApp         , this, &M2MResourceManager::killApp  );

    connect(this, &M2MResourceManager::killAllObjects, extSocket, &M2MLocalSocket::killAllObjects);

    connect(extSocket, SIGNAL(destroyed(QObject*)), extSocketThrd, SLOT(quit()));
    connect(extSocketThrd, SIGNAL(finished()), extSocketThrd, SLOT(deleteLater()));



//    connect(extSocket, &M2MLocalSocket::command4dev     , zbyrator, &MeterManager::command4devStr      );

//    extSocketThrd->start();
    connect(this, SIGNAL(goGoGo()), extSocketThrd, SLOT(start()));

    return extSocket;

}

//----------------------------------------------------------------------

void M2MResourceManager::createSharedMemory(const bool &verboseMode)
{
    QThread *writerthred = new QThread;
    writerthred->setObjectName("HTTPAppLogs");

    M2MAppLogs *writer = new M2MAppLogs(
                SharedMemoHelper::defM2MServerAppMemoName(),
                SharedMemoHelper::defM2MServerAppSemaName(),
                "", 2222, 60000, verboseMode);

    writer->mymaximums.write2ram = 120;
    writer->mymaximums.write2file = 250;

    writer->moveToThread(writerthred);
    connect(writer, SIGNAL(destroyed(QObject*)), writerthred, SLOT(quit()));
    connect(writerthred, SIGNAL(finished()), writerthred, SLOT(deleteLater()));
    connect(writerthred, SIGNAL(started()), writer, SLOT(initObjectLtr()));

    connect(this, &M2MResourceManager::killAllObjects, writer, &SharedMemoWriter::flushAllNowAndDie);

//    connect(this, &M2MResourceManager::add2systemLogError, writer, &M2MAppLogs::add2systemLogError);
    connect(this, &M2MResourceManager::addEvent2log, writer, &M2MAppLogs::add2systemLogEvent);
//    connect(this, &HTTPResourceManager::add2systemLogWarn , writer, &HTTPAppLogs::add2systemLogWarn);

    connect(this, SIGNAL(goGoGo()), writerthred, SLOT(start()));


//    writerthred->start();
//    return writer;
}

//----------------------------------------------------------------------

M2MSharedMemoryWriter *M2MResourceManager::createSharedMemoryWriter(const bool &verboseMode)
{
    //    case MTD_EXT_NAME_SVAHA_SERVICE         : sharedMemoKey = SharedMemoHelper::defSvahaServerMemoName()    ; semaName = SharedMemoHelper::defSvahaServerSemaName()           ; break;

    M2MSharedMemoryWriter *writer = new M2MSharedMemoryWriter(
                SharedMemoHelper::defSvahaServerMemoName(),
                SharedMemoHelper::defSvahaServerSemaName(),
                verboseMode);

    QThread *thread = new QThread;
    thread->setObjectName("M2MSharedMemoryWriter");
    writer->moveToThread(thread);

    connect(thread, SIGNAL(started()), writer, SLOT(onThreadStarted()));
    connect(writer, SIGNAL(destroyed(QObject*)), thread, SLOT(quit()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));





    connect(this, &M2MResourceManager::killAllObjects, writer, &M2MSharedMemoryWriter::flushAllNowAndDie);

//    thread->start();
    connect(this, SIGNAL(goGoGo()), thread, SLOT(start()));

    return writer;
}

//----------------------------------------------------------------------

M2MConnHolderServer *M2MResourceManager::createM2MServer(const bool &verboseMode, M2MSharedMemoryWriter *writer, M2MLocalSocket *extSocket)
{
    M2MConnHolderServer *server = new M2MConnHolderServer(verboseMode);

    QThread *thread = new QThread;
    thread->setObjectName("M2MConnHolderServer");
    server->moveToThread(thread);


    connect(thread, SIGNAL(started()), server, SLOT(onThreadStarted()));
    connect(server, SIGNAL(destroyed(QObject*)), thread, SLOT(quit()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

//shared memory
    connect(server, &M2MConnHolderServer::onConnectionTableChanged, writer, &M2MSharedMemoryWriter::onConnectionTableChanged);
    connect(server, &M2MConnHolderServer::onConnectionTableData, writer, &M2MSharedMemoryWriter::onConnectionTableData);

    connect(writer, &M2MSharedMemoryWriter::sendConnectionTable, server, &M2MConnHolderServer::sendConnectionTable);

    //settings
    connect(this, &M2MResourceManager::setTimeouts, server, &M2MConnHolderServer::setTimeouts);
    connect(this, &M2MResourceManager::setBackupWorkDirectory, server, &M2MConnHolderServer::setBackupWorkDirectory);
    connect(this, &M2MResourceManager::setDataConnectionParams, server, &M2MConnHolderServer::setDataConnectionParams);

    connect(this, &M2MResourceManager::setServicePortSmart, server, &M2MConnHolderServer::setServicePortSmart);

    connect(this, &M2MResourceManager::setAllowAndBlockList, server, &M2MConnHolderServer::setAllowAndBlockList);

     //local socket

    connect(extSocket, &M2MLocalSocket::killClientsNow, server, &M2MConnHolderServer::killClientsNow);


    connect(this, &M2MResourceManager::killAllObjects, server, &M2MConnHolderServer::stopAllSlot);//, &M2MSharedMemoryWriter::flushAllNowAndDie);

    connect(server, &M2MConnHolderServer::onFailed2startServer, this, &M2MResourceManager::onFailed2startServer);
    connect(server, &M2MConnHolderServer::addEvent2log, this, &M2MResourceManager::addEvent2log);

    connect(server, &M2MConnHolderServer::gimmeSettings, this, &M2MResourceManager::reloadSettings);

//    thread->start();
    connect(this, SIGNAL(goGoGo()), thread, SLOT(start()));

    return server;
}

//----------------------------------------------------------------------

M2MBackupManager *M2MResourceManager::createBackupManager(const bool &verboseMode, M2MConnHolderServer *server)
{

    M2MBackupManager *manager = new M2MBackupManager(verboseMode);

    QThread *thread = new QThread;
    thread->setObjectName("M2MBackupManager");
    manager->moveToThread(thread);



    connect(thread, SIGNAL(started()), manager, SLOT(onThreadStarted()));
    connect(manager, SIGNAL(destroyed(QObject*)), thread, SLOT(quit()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));


    connect(server, &M2MConnHolderServer::onSyncRequestRemoteSha1isEqual, manager, &M2MBackupManager::onSyncRequestRemoteSha1isEqual);
    connect(server, &M2MConnHolderServer::onSyncFileDownloaded          , manager, &M2MBackupManager::onSyncFileDownloaded);
    connect(server, &M2MConnHolderServer::onConnectedTheseMacs          , manager, &M2MBackupManager::onConnectedTheseMacs);
    connect(server, &M2MConnHolderServer::onDisconnectedTheseMacs       , manager, &M2MBackupManager::onDisconnectedTheseMacs);

    connect(manager, &M2MBackupManager::checkBackup4thisMac, server, &M2MConnHolderServer::checkBackup4thisMac);

    connect(this, &M2MResourceManager::setSyncParams    , manager, &M2MBackupManager::setSyncParams);


    connect(manager, &M2MBackupManager::gimmeSettings, this, &M2MResourceManager::reloadSettings);
//    connect(this, SIGNAL(checkSett2all())                                    , backup, SLOT(reloadSettings())                                    );

    connect(this, SIGNAL(goGoGo()), thread, SLOT(start()));
    return manager;
}


//----------------------------------------------------------------------
