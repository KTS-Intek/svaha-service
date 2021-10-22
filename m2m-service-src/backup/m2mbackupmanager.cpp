#include "m2mbackupmanager.h"


///[!] svaha-service
#include "m2m-service-src/backup/m2moldbackupskiller.h"
#include "m2m-service-src/backup/m2mlocalfilesha1checker.h"



#include "m2m-server-define.h"


#include <QThread>
#include <QTimer>
#include <QDebug>


//--------------------------------------------------------------------------------------------

M2MBackupManager::M2MBackupManager(const bool &verboseMode, QObject *parent) : QObject(parent)
{
    myParams.verboseMode = verboseMode;

}

//--------------------------------------------------------------------------------------------

qint64 M2MBackupManager::msec4check(qint64 &currMsec)
{
    const QDateTime currDtUtc = QDateTime::currentDateTimeUtc();
    qint64 msec = 0;
    switch(syncParams.syncMode){
    case DT_MODE_EVERY_DAY  : msec = currDtUtc.addDays(-1).toMSecsSinceEpoch()  ; break;
    case DT_MODE_EVERY_WEEK : msec = currDtUtc.addDays(-7).toMSecsSinceEpoch()  ; break;
    case DT_MODE_EVERY_MONTH: msec = currDtUtc.addMonths(-1).toMSecsSinceEpoch(); break;
    }

    currMsec = currDtUtc.toMSecsSinceEpoch();
    return msec;
}

//--------------------------------------------------------------------------------------------

bool M2MBackupManager::isItTime2makeASyncRequest(const QString &mac, const qint64 &msec, const qint64 &currMsec, QString &lastSha1)
{
    const Mac2syncInfo info = myParams.hMac2syncInfo.value(mac);
    lastSha1 = info.lastSha1base64;

//    if(info.dtSyncRequest.isValid() && info.dtSyncRequest.secsTo(currDtUtc) < (3 * 3600))
    if(info.msecSyncRequest > 0 && (info.msecSyncRequest - currMsec) < (3 * 3600))
        return false;//даю час на синхронізацію

//    return (!info.dtLastSyncFile.isValid() || info.dtLastSyncFile <= dt);
    return (info.msecLastSyncFile < 1|| info.msecLastSyncFile <= msec);

}

QStringList M2MBackupManager::removeOldConnections(QHash<qint64, QStringList> &hDateConnected2macLst, QHash<QString, qint64> &hConnectedMac2date, quint32 currConnCounter, const quint32 &maxConnSize, QHash<QString, bool> &hAliveMacConn)
{
    QStringList killedMacs;
    //видаляю старі з'єднання, крім тих що використовуються
    QList<qint64> lk = hDateConnected2macLst.keys();
    std::sort(lk.begin(), lk.end());

    for(int i = 0; i < 10000 && !lk.isEmpty() && currConnCounter >= maxConnSize; i++){
        QStringList macL = hDateConnected2macLst.value(lk.at(i));
        QStringList l;
        for(int j = 0, jMax = macL.size(); j < jMax; j++){
            const QString mac = macL.at(j);

            if(hAliveMacConn.contains(mac)){
                l.append(mac);
            }else{
                killedMacs.append(mac);
                currConnCounter--;
                hConnectedMac2date.remove(mac);
                hAliveMacConn.remove(mac);
            }
        }

        if(l.isEmpty())
            hDateConnected2macLst.remove(lk.at(i));
        else
            hDateConnected2macLst.insert(lk.at(i), l);
    }
    return killedMacs;
}

//--------------------------------------------------------------------------------------------


void M2MBackupManager::onThreadStarted()
{
    if(myParams.verboseMode)
        qDebug() << "BackUpManager::onThreadStarted() ";

    createOldBackupKiller();


    QTimer *tmrQueue = new QTimer(this);
    tmrQueue->setSingleShot(true);
    connect(tmrQueue, SIGNAL(timeout()), this, SLOT(onCheckQueueSha1LocalFs()) );
    connect(this, SIGNAL(tmrQueueStart(int)), tmrQueue, SLOT(start(int)) );

    emit tmrQueueStart(11111);


//    emit gimmeSettings();
}

//--------------------------------------------------------------------------------------------

void M2MBackupManager::createOldBackupKiller()
{
    M2MOldBackupsKiller *killer = new M2MOldBackupsKiller(myParams.verboseMode);
    QThread *thread = new QThread;
    thread->setObjectName("M2MOldBackupsKiller");
    killer->moveToThread(thread);

    connect(thread, SIGNAL(started()), killer, SLOT(onThreadStarted()));
    connect(killer, SIGNAL(destroyed(QObject*)), thread, SLOT(quit()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    connect(this, &M2MBackupManager::kickOffAll, killer, &M2MOldBackupsKiller::stopAllAndDie);


    connect(this, &M2MBackupManager::checkRemovedMacs, killer, &M2MOldBackupsKiller::checkRemovedMacs);
    connect(this, &M2MBackupManager::setMaxSett, killer, &M2MOldBackupsKiller::setMaxSett);


    thread->start();


}

//--------------------------------------------------------------------------------------------

void M2MBackupManager::stopAllAndDie()
{
    emit kickOffAll();
    deleteLater();
}

//--------------------------------------------------------------------------------------------

void M2MBackupManager::setSyncParams(QString workDir, quint8 syncMode, quint8 maxFileCount, quint32 maxSizeMacTable,
                                     quint32 maxCountSha1LocalFsParallel, qint32 maxSizeSyncRequest, quint32 maxCountSyncRequestParallel,
                                     qint32 maxYearSave, qint32 minUniqMacs)
{
    syncParams.workDir = workDir;
    syncParams.syncMode = syncMode;
    syncParams.maxFileCount = maxFileCount;
    syncParams.maxSizeMacTable = maxSizeMacTable;

    syncParams.maxCountSha1LocalFsParallel = maxCountSha1LocalFsParallel;
    syncParams.maxSizeSyncRequest = maxSizeSyncRequest;
    syncParams.maxCountSyncRequestParallel = maxCountSyncRequestParallel;

    syncParams.maxYearSave = maxYearSave;
    syncParams.minUniqMacs = minUniqMacs;
//    void setMaxSett(qint32 maxYearSave, qint32 maxMacsSave, qint32 minUniqNumb, QString workDir);

    emit setMaxSett(syncParams.maxYearSave, syncParams.maxFileCount,  syncParams.minUniqMacs, syncParams.workDir);
}

//--------------------------------------------------------------------------------------------

void M2MBackupManager::onOneDestFromQueueSha1LocalFs()
{
    //вказує на те скільки сканерів локльної ФС зараз запущено, не чіпати
    if(myParams.currSha1LocalFsMacQueue > 0)
        myParams.currSha1LocalFsMacQueue--;
}

//--------------------------------------------------------------------------------------------

void M2MBackupManager::setLocalMacDateSha1(QStringList macL, QStringList sha1L, QList<qint64> msecCreatedL, int counter)
{
    //після перевірки локальної ФС якщо було виявлено файл резервної копії, то фіксую дату створення, та додаю його до загальної черги перевірки на синхронізацію
    for(int i = 0; i < counter && i < MAX_MAC_INLIST; i++){

        const QString mac = macL.at(i);

        if(!myParams.hConnectedMac2date.contains(mac))//якщо МАК був видалений з таблиці
            continue;

        if(!myParams.listKeys4hMac2syncInfo.contains(mac))
            myParams.listKeys4hMac2syncInfo.append(mac);

        Mac2syncInfo info = myParams.hMac2syncInfo.value(mac);
        info.lastSha1base64 = sha1L.at(i);
        info.msecLastSyncFile = msecCreatedL.at(i);// dtCreatedUtcL.at(i);
        myParams.hMac2syncInfo.insert(mac, info);
    }
}

//--------------------------------------------------------------------------------------------

void M2MBackupManager::appendMac2queueSyncRequest(QStringList macL, int counter)
{
    //у цих МАКів відсутні резервні копії в локальній ФС,
    //додаю їх у загальну чергу але дата - невідома

    for(int i = 0; i < counter && MAX_MAC_INLIST; i++){

        const QString mac = macL.at(i);

        if(!myParams.hConnectedMac2date.contains(mac))//якщо МАК був видалений з таблиці
            continue;

        if(!myParams.listKeys4hMac2syncInfo.contains(mac))
            myParams.listKeys4hMac2syncInfo.append(mac);

        Mac2syncInfo info = myParams.hMac2syncInfo.value(mac);
        info.lastSha1base64 = "";
        info.msecLastSyncFile = 0;// QDateTime();
        myParams.hMac2syncInfo.insert(mac, info);
    }

}

//--------------------------------------------------------------------------------------------
//обробка з'єднань/від'єднань

void M2MBackupManager::onConnectedTheseMacs(QStringList macL)
{
    //після підключення клієнтів, яким дозволена синхронізація перевіряю чи є дані по синхронізації у локальній ФС
    bool hasNewMacs = false;
    qint64 dt = QDateTime::currentMSecsSinceEpoch();

    for(int i = 0, iMax = macL.size(); i < iMax && i < MAX_MAC_INLIST; i++){
        const QString mac = macL.at(i);

        if(myParams.hConnectedMac2date.contains(mac)){
            //такий мак уже є, його необхідно видалити з hDateConnected2macLst
            QStringList l = myParams.hDateConnected2macLst.value(myParams.hConnectedMac2date.value(mac));
            l.removeOne(mac);
            if(l.isEmpty())//список порожній видаляю рядок
                myParams.hDateConnected2macLst.remove(myParams.hConnectedMac2date.value(mac));
            else//вставляю оновлений список
                myParams.hDateConnected2macLst.insert(myParams.hConnectedMac2date.value(mac), l);
        }else{
            hasNewMacs = true;
        }

        myParams.hAliveMacConn.insert(mac, true);//позначаю з'єднання як живе
        myParams.hConnectedMac2date.insert(mac, dt);//мітка дати під'єднання

        myParams.wait4answerSyncQueue.remove(mac);//видаляю з черги очікування, на всяк випадок

        if(!myParams.hMac2syncInfo.contains(mac) && //у мене відсутній кеш дані по цьому МАКу
                !myParams.checkSha1LocalFsMacQueue.contains(mac)){//і він відсутній в черзі на перевірку
            myParams.checkSha1LocalFsMacQueue.append(mac);
//            hasNewMacs = true;
        }


    }
    if(myParams.hDateConnected2macLst.contains(dt)){
        macL.append(myParams.hDateConnected2macLst.value(dt));
        macL.removeDuplicates();
    }
    myParams.hDateConnected2macLst.insert(dt, macL);
    if(!hasNewMacs)
        return;

    const quint32 macCounter = myParams.hConnectedMac2date.size();
    if(macCounter >= syncParams.maxSizeMacTable){
        //необхідно видалити застарілі записи, але тільки ті що не використовуються
        const QStringList l = removeOldConnections(myParams.hDateConnected2macLst, myParams.hConnectedMac2date, macCounter, syncParams.maxSizeMacTable, myParams.hAliveMacConn);

        for(int i = 0, iMax = l.size(); i < iMax; i++){
            const QString mac = l.at(i);

            myParams.wait4answerSyncQueue.remove(mac);
            myParams.hAliveMacConn.remove(mac);
            myParams.hMac2syncInfo.remove(mac);
            myParams.listKeys4hMac2syncInfo.removeOne(mac);
            myParams.syncRequestQueue.removeOne(mac);
            myParams.checkSha1LocalFsMacQueue.removeOne(mac);
        }
    }
}

//--------------------------------------------------------------------------------------------

void M2MBackupManager::onDisconnectedTheseMacs(QStringList macL, int counter)
{
    for(int i = 0; i < counter && i < MAX_MAC_INLIST; i++)
        onDisconnectedThisMac(macL.at(i));
}

//--------------------------------------------------------------------------------------------

void M2MBackupManager::onDisconnectedThisMac(QString mac)
{
    myParams.hAliveMacConn.remove(mac);
    myParams.wait4answerSyncQueue.remove(mac);
}

//--------------------------------------------------------------------------------------------

void M2MBackupManager::onSyncRequestRemoteSha1isEqual(QStringList macL)
{
    qint64 dt = QDateTime::currentMSecsSinceEpoch();

    for(int i = 0, iMax = macL.size(); i < iMax; i++){
        const QString mac = macL.at(i);

        myParams.wait4answerSyncQueue.remove(mac);
        myParams.syncRequestQueue.removeOne(mac);

        if(!myParams.hMac2syncInfo.contains(mac))
            continue;

        if(!myParams.hConnectedMac2date.contains(mac))
            continue;

        Mac2syncInfo info = myParams.hMac2syncInfo.value(mac);
        info.msecSyncRequest = dt;
        myParams.hMac2syncInfo.insert(mac, info);
    }
}

//--------------------------------------------------------------------------------------------

void M2MBackupManager::onSyncFileDownloaded(QStringList macL, QString lastSha1base64, qint64 msecCreatedUtc)
{
    int iMax = macL.size();
    for(int i = 0; i < iMax; i++){
        const QString mac = macL.at(i);


        myParams.wait4answerSyncQueue.remove(mac);
        myParams.syncRequestQueue.removeOne(mac);

        if(!myParams.hMac2syncInfo.contains(mac))
            continue;

        if(!myParams.hConnectedMac2date.contains(mac))
            continue;

        Mac2syncInfo info = myParams.hMac2syncInfo.value(mac);
        info.msecSyncRequest = 0;//QDateTime();
        info.lastSha1base64 = lastSha1base64;
        info.msecLastSyncFile = msecCreatedUtc;
        myParams.hMac2syncInfo.insert(mac, info);
    }

    emit checkRemovedMacs(macL, iMax);
}

//--------------------------------------------------------------------------------------------

void M2MBackupManager::onCheckQueueSha1LocalFs()
{
    const int msec = (checkQueueSha1LocalFs()) ? 1111 : 111;

    emit tmrQueueStart(msec);


}

//--------------------------------------------------------------------------------------------

bool M2MBackupManager::checkQueueSha1LocalFs()
{
    if(syncParams.workDir.isEmpty()){
        emit gimmeSettings();
        return true; //long timeout
    }
    qint64 currMsec;// currDtUtc;
    myParams.msecLastCheck = msec4check(currMsec);

    //перевіряю чергу на сканування резервних копій в локальній ФС
    QStringList macGroupList;
    for(int macl = 0, i = 0 ; i < 100000000 && myParams.currSha1LocalFsMacQueue < syncParams.maxCountSha1LocalFsParallel && !myParams.checkSha1LocalFsMacQueue.isEmpty(); i++){
        const QString mac = myParams.checkSha1LocalFsMacQueue.takeFirst();

        if(myParams.hMac2syncInfo.value(mac).msecLastSyncFile < 1){ // .dtLastSyncFile.isValid()){//тобто ще не було перевірки, у hMac2syncInfo є тільки ті що пройшли перевірку в локальній ФС
            macl++;
            macGroupList.append(mac);

            if(macl > MAX_MAC_GROUPS){
                macl = 0;
                startCheckMacGroupSmart(macGroupList);
            }
        }
    }

    if(!macGroupList.isEmpty())
        startCheckMacGroupSmart(macGroupList);


    //відправка запиту на перевірку ХЕШ суми останньої резервної копії, якщо не збігається то має переслати сюди файл
    //  currSyncRequestCount;//current parallel requests count and wait4answerSyncQueue.size
    if(myParams.syncRequestQueue.isEmpty())
        myParams.syncRequestQueue = myParams.listKeys4hMac2syncInfo;

    for(quint32 currSyncRequestCount = myParams.wait4answerSyncQueue.size() ; currSyncRequestCount < syncParams.maxCountSyncRequestParallel && !myParams.syncRequestQueue.isEmpty(); ){
        const QString mac = myParams.syncRequestQueue.takeFirst();

        if(!myParams.hAliveMacConn.contains(mac) || myParams.wait4answerSyncQueue.contains(mac))
            continue;

        QString lastSha1;
        if(isItTime2makeASyncRequest(mac, myParams.msecLastCheck, currMsec, lastSha1)){
            myParams.wait4answerSyncQueue.insert(mac, true);
            currSyncRequestCount++;
            emit checkBackup4thisMac(mac, lastSha1);
        }
    }


    return (myParams.currSha1LocalFsMacQueue == 0 || myParams.currSha1LocalFsMacQueue == syncParams.maxCountSha1LocalFsParallel);
//        msec = 1111;

//    emit tmrQueueStart(msec);



}

//--------------------------------------------------------------------------------------------

void M2MBackupManager::startCheckMacGroupSmart(QStringList &macL)
{
    myParams.currSha1LocalFsMacQueue++;
    startCheckMacGroup(macL);
    macL.clear();
}

//--------------------------------------------------------------------------------------------

void M2MBackupManager::startCheckMacGroup(const QStringList &macL)
{

    QThread *t = new QThread;
    t->setObjectName("");

    M2MLocalFileSha1Checker *c = new M2MLocalFileSha1Checker(myParams.verboseMode);
    c->moveToThread(t);

    c->setSettings(macL, syncParams.workDir, syncParams.syncMode, syncParams.maxFileCount);


    connect(c, SIGNAL(destroyed(QObject*)), t, SLOT(quit())   );
    connect(t, SIGNAL(finished()), t, SLOT(deleteLater())     );
    connect(t, SIGNAL(started()) , c, SLOT(onThreadStarted()) );

    connect(c, SIGNAL(destroyed(QObject*))                          , this, SLOT(onOneDestFromQueueSha1LocalFs()));
    connect(c, &M2MLocalFileSha1Checker::appendMac2queueSyncRequest , this, &M2MBackupManager::appendMac2queueSyncRequest);
    connect(c, &M2MLocalFileSha1Checker::setLocalMacDateSha1        , this, &M2MBackupManager::setLocalMacDateSha1);
    connect(c, &M2MLocalFileSha1Checker::checkRemovedMacs           , this, &M2MBackupManager::checkRemovedMacs);


    connect(this, &M2MBackupManager::kickOffAll, c, &M2MLocalFileSha1Checker::deleteLater);


    t->start();


}

//--------------------------------------------------------------------------------------------
