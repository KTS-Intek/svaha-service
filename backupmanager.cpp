#include "backupmanager.h"
#include "settloader4svaha.h"
#include "checklocalfilesha1.h"

#include "svahadefine.h"
#include "oldbackupcleaner.h"

//---------------------------------------------------------------------------------------------------
BackUpManager::BackUpManager(const bool &verboseMode, QObject *parent) : QObject(parent)
{
    this->verboseMode = verboseMode;
    currSha1LocalFsMacQueue = 0;
}
//---------------------------------------------------------------------------------------------------
void BackUpManager::onThreadStarted()
{
    if(verboseMode)
        qDebug() << "BackUpManager::onThreadStarted() ";

    if(true){
        QDateTime d;
        dtLastCheck = dt4check(d);
    }
    reloadSettings();

    if(true){
        QTimer *tmrQueue = new QTimer(this);
        tmrQueue->setSingleShot(true);
        connect(tmrQueue, SIGNAL(timeout()), this, SLOT(onCheckQueueSha1LocalFs()) );
        connect(this, SIGNAL(tmrQueueStart(int)), tmrQueue, SLOT(start(int)) );
        emit tmrQueueStart(111);
    }

    if(true){
        OldBackupCleaner *c = new OldBackupCleaner(syncSett.maxYearSave, syncSett.maxFileCount, syncSett.minUniqMacs, syncSett.workDir);
        QThread *t = new QThread(this);
        c->moveToThread(t);

        connect(t, SIGNAL(started()), c, SLOT(onThreadStarted()) );

        connect(this, SIGNAL(checkRemovedMacs(QStringList,int)), c, SLOT(checkRemovedMacs(QStringList,int)) );
        connect(this, SIGNAL(setMaxSett(qint32,qint32,qint32,QString)), c, SLOT(setMaxSett(qint32,qint32,qint32,QString)) );

        t->start();
    }
}
//---------------------------------------------------------------------------------------------------
void BackUpManager::reloadSettings()
{
    /*
        QString workDir;
        quint8 syncMode;
        quint8 maxFileCount;

        quint32 maxSizeMacTable;//maximum Mac count, обмежує hConnectedMac2date

        quint32 maxCountSha1LocalFsParallel;//maximum thread for check sha1


        qint32 maxSizeSyncRequest; //maximum sync request list size
        quint32 maxCountSyncRequestParallel; //maximum request at the same time
*/
    SettLoader4svaha sLoader;
    syncSett.workDir = sLoader.loadOneSett(SETT_SYNC_WORKDIR).toString();
    syncSett.syncMode = sLoader.loadOneSett(SETT_SYNC_MODE).toUInt();
    syncSett.maxFileCount = sLoader.loadOneSett(SETT_SYNC_MAX_FILE_COUNT).toUInt();
    syncSett.maxSizeMacTable = sLoader.loadOneSett(SETT_SYNC_MAX_SIZE_MAC_TABLE).toUInt();
    syncSett.maxCountSha1LocalFsParallel = sLoader.loadOneSett(SETT_SYNC_MAX_COUNT_SHA1_CHRSPRLL).toUInt();
    syncSett.maxSizeSyncRequest = sLoader.loadOneSett(SETT_SYNC_MAX_SIZE_SYNC_REQUEST).toInt();
    syncSett.maxCountSyncRequestParallel = sLoader.loadOneSett(SETT_SYNC_MAX_COUNT_SYNQ_RQSTPRLL).toUInt();

    syncSett.maxYearSave = sLoader.loadOneSett(SETT_SYNC_MAX_YEAR_SAVE).toInt();
    syncSett.minUniqMacs = sLoader.loadOneSett(SETT_SYNC_MIN_UNIQ_MAC_FILES).toInt();

    emit setMaxSett(syncSett.maxYearSave, syncSett.maxFileCount, syncSett.minUniqMacs, syncSett.workDir );

}
//---------------------------------------------------------------------------------------------------

//обробка з'єднань/від'єднань
void BackUpManager::onConnectedThisMacs(QStringList macL)
{
    //після підключення клієнтів, яким дозволена синхронізація перевіряю чи є дані по синхронізації у локальній ФС
    bool hasNewMacs = false;
    qint64 dt = QDateTime::currentMSecsSinceEpoch();

    for(int i = 0, iMax = macL.size(); i < iMax && i < MAX_MAC_INLIST; i++){

        if(hConnectedMac2date.contains(macL.at(i))){
            //такий мак уже є, його необхідно видалити з hDateConnected2macLst
            QStringList l = hDateConnected2macLst.value(hConnectedMac2date.value(macL.at(i)));
            l.removeOne(macL.at(i));
            if(l.isEmpty())//список порожній видаляю рядок
                hDateConnected2macLst.remove(hConnectedMac2date.value(macL.at(i)));
            else//вставляю оновлений список
                hDateConnected2macLst.insert(hConnectedMac2date.value(macL.at(i)), l);
        }else{
            hasNewMacs = true;
        }

        hAliveMacConn.insert(macL.at(i), true);//позначаю з'єднання як живе
        hConnectedMac2date.insert(macL.at(i), dt);//мітка дати під'єднання

        wait4answerSyncQueue.remove(macL.at(i));//видаляю з черги очікування, на всяк випадок

        if(!hMac2syncInfo.contains(macL.at(i)) && //у мене відсутній кеш дані по цьому МАКу
                !checkSha1LocalFsMacQueue.contains(macL.at(i))){//і він відсутній в черзі на перевірку
            checkSha1LocalFsMacQueue.append(macL.at(i));
//            hasNewMacs = true;
        }


    }
    if(hDateConnected2macLst.contains(dt)){
        macL.append(hDateConnected2macLst.value(dt));
        macL.removeDuplicates();
    }
    hDateConnected2macLst.insert(dt, macL);
    if(!hasNewMacs)
        return;

    quint32 macCounter = hConnectedMac2date.size();
    if(macCounter >= syncSett.maxSizeMacTable){
        //необхідно видалити застарілі записи, але тільки ті що не використовуються
        QStringList l = removeOldConnections(hDateConnected2macLst, hConnectedMac2date, macCounter, syncSett.maxSizeMacTable,hAliveMacConn);

        for(int i = 0, iMax = l.size(); i < iMax; i++){
            wait4answerSyncQueue.remove(l.at(i));
            hAliveMacConn.remove(l.at(i));
            hMac2syncInfo.remove(l.at(i));
            listKeys4hMac2syncInfo.removeOne(l.at(i));
            syncRequestQueue.removeOne(l.at(i));
            checkSha1LocalFsMacQueue.removeOne(l.at(i));
        }
    }
}
//---------------------------------------------------------------------------------------------------
void BackUpManager::onDisconnectedThisMacs(QStringList macL, int counter)
{
    for(int i = 0; i < counter && i < MAX_MAC_INLIST; i++)
        onDisconnectedThisMac(macL.at(i));
}
//---------------------------------------------------------------------------------------------------
void BackUpManager::onDisconnectedThisMac(QString mac)
{
    hAliveMacConn.remove(mac);
    wait4answerSyncQueue.remove(mac);
}





//---------------------------------------------------------------------------------------------------

void BackUpManager::setLocalMacDateSha1(QStringList macL, QStringList sha1L, QList<QDateTime> dtCreatedUtcL, int counter)
{
    //після перевірки локальної ФС якщо було виявлено файл резервної копії, то фіксую дату створення, та додаю його до загальної черги перевірки на синхронізацію
    for(int i = 0; i < counter && i < MAX_MAC_INLIST; i++){

        if(!hConnectedMac2date.contains(macL.at(i)))//якщо МАК був видалений з таблиці
            continue;

        if(!listKeys4hMac2syncInfo.contains(macL.at(i)))
            listKeys4hMac2syncInfo.append(macL.at(i));

        Mac2syncInfo info = hMac2syncInfo.value(macL.at(i));
        info.lastSha1base64 = sha1L.at(i);
        info.dtLastSyncFile = dtCreatedUtcL.at(i);
        hMac2syncInfo.insert(macL.at(i), info);
    }
}

//---------------------------------------------------------------------------------------------------

void BackUpManager::appendMac2queueSyncRequest(QStringList macL, int counter)
{
//у цих МАКів відсутні резервні копії в локальній ФС,
//додаю їх у загальну чергу але дата - невідома

    for(int i = 0; i < counter && MAX_MAC_INLIST; i++){
        if(!hConnectedMac2date.contains(macL.at(i)))//якщо МАК був видалений з таблиці
            continue;

        if(!listKeys4hMac2syncInfo.contains(macL.at(i)))
            listKeys4hMac2syncInfo.append(macL.at(i));

        Mac2syncInfo info = hMac2syncInfo.value(macL.at(i));
        info.lastSha1base64 = "";
        info.dtLastSyncFile = QDateTime();
        hMac2syncInfo.insert(macL.at(i), info);
    }


}
//---------------------------------------------------------------------------------------------------




void BackUpManager::onCheckQueueSha1LocalFs()
{
    int msec = 111;
    QDateTime currDtUtc;
    dtLastCheck = dt4check(currDtUtc);

//перевіряю чергу на сканування резервних копій в локальній ФС
    QStringList macGroupList;
    for(int macl = 0 ; currSha1LocalFsMacQueue < syncSett.maxCountSha1LocalFsParallel && !checkSha1LocalFsMacQueue.isEmpty(); ){
        QString mac = checkSha1LocalFsMacQueue.takeFirst();

        if(!hMac2syncInfo.value(mac).dtLastSyncFile.isValid()){//тобто ще не було перевірки, у hMac2syncInfo є тільки ті що пройшли перевірку в локальній ФС
            macl++;
            macGroupList.append(mac);

            if(macl > MAX_MAC_GROUPS){
                macl = 0;
                currSha1LocalFsMacQueue++;
                startCheckMacGroup(macGroupList);
                macGroupList.clear();
            }
        }
    }

    if(!macGroupList.isEmpty()){
        currSha1LocalFsMacQueue++;
        startCheckMacGroup(macGroupList);
    }




    //відправка запиту на перевірку ХЕШ суми останньої резервної копії, якщо не збігається то має переслати сюди файл
    //  currSyncRequestCount;//current parallel requests count and wait4answerSyncQueue.size
    if(syncRequestQueue.isEmpty())
        syncRequestQueue = listKeys4hMac2syncInfo;

    for(quint32 currSyncRequestCount = wait4answerSyncQueue.size() ; currSyncRequestCount < syncSett.maxCountSyncRequestParallel && !syncRequestQueue.isEmpty(); ){
        QString mac = syncRequestQueue.takeFirst();
        if(!hAliveMacConn.contains(mac) || wait4answerSyncQueue.contains(mac))
            continue;
        QString lastSha1;
        if(isNeed2syncRequest(mac, dtLastCheck, currDtUtc, lastSha1)){
            wait4answerSyncQueue.insert(mac, true);
            currSyncRequestCount++;
            emit checkBackup4thisMac(mac, lastSha1);
        }
    }

    if(currSha1LocalFsMacQueue == 0 || currSha1LocalFsMacQueue == syncSett.maxCountSha1LocalFsParallel)
        msec = 1111;

    emit tmrQueueStart(msec);

}
//---------------------------------------------------------------------------------------------------
void BackUpManager::onOneDestFromQueueSha1LocalFs()
{
    //вказує на те скільки сканерів локльної ФС зараз запущено, не чіпати
    if(currSha1LocalFsMacQueue > 0)
        currSha1LocalFsMacQueue--;
}
//---------------------------------------------------------------------------------------------------
void BackUpManager::onSyncRequestRemoteSha1isEqual(QStringList macL)
{
    QDateTime dt = QDateTime::currentDateTimeUtc();
    for(int i = 0, iMax = macL.size(); i < iMax; i++){
        wait4answerSyncQueue.remove(macL.at(i));
        syncRequestQueue.removeOne(macL.at(i));

        if(!hMac2syncInfo.contains(macL.at(i)))
            continue;

        if(!hConnectedMac2date.contains(macL.at(i)))
            continue;

        Mac2syncInfo info = hMac2syncInfo.value(macL.at(i));
        info.dtSyncRequest = dt;
        hMac2syncInfo.insert(macL.at(i), info);
    }
}
//---------------------------------------------------------------------------------------------------
void BackUpManager::onSyncFileDownloaded(QStringList macL, QString lastSha1base64, QDateTime dtCreatedUtc)
{
    int iMax = macL.size();
    for(int i = 0; i < iMax; i++){
        wait4answerSyncQueue.remove(macL.at(i));
        syncRequestQueue.removeOne(macL.at(i));

        if(!hMac2syncInfo.contains(macL.at(i)))
            continue;

        if(!hConnectedMac2date.contains(macL.at(i)))
            continue;

        Mac2syncInfo info = hMac2syncInfo.value(macL.at(i));
        info.dtSyncRequest = QDateTime();
        info.lastSha1base64 = lastSha1base64;
        info.dtLastSyncFile = dtCreatedUtc;
        hMac2syncInfo.insert(macL.at(i), info);
    }

    emit checkRemovedMacs(macL, iMax);
}

//---------------------------------------------------------------------------------------------------
QDateTime BackUpManager::dt4check(QDateTime &currDtUtc)
{
    QDateTime dt = currDtUtc = QDateTime::currentDateTimeUtc();
    switch(syncSett.syncMode){
    case DT_MODE_EVERY_DAY  : dt = currDtUtc.addDays(-1)  ; break;
    case DT_MODE_EVERY_WEEK : dt = currDtUtc.addDays(-7)  ; break;
    case DT_MODE_EVERY_MONTH: dt = currDtUtc.addMonths(-1); break;
    }
    return dt;
}
//---------------------------------------------------------------------------------------------------
bool BackUpManager::isNeed2syncRequest(const QString &mac, const QDateTime &dt, const QDateTime &currDtUtc, QString &lastSha1)
{
    Mac2syncInfo info = hMac2syncInfo.value(mac);
    lastSha1 = info.lastSha1base64;
    if(info.dtSyncRequest.isValid() && info.dtSyncRequest.secsTo(currDtUtc) < (3 * 3600))
        return false;//даю час на синхронізацію
    return (!info.dtLastSyncFile.isValid() || info.dtLastSyncFile <= dt);
}
//---------------------------------------------------------------------------------------------------
void BackUpManager::startCheckMacGroup(const QStringList &macL)
{
    QThread *t = new QThread(this);

    CheckLocalFileSha1 *c = new CheckLocalFileSha1(macL, syncSett.workDir, syncSett.syncMode, syncSett.maxFileCount);
    c->moveToThread(t);
    connect(c, SIGNAL(destroyed(QObject*)), t, SLOT(quit())   );
    connect(t, SIGNAL(finished()), t, SLOT(deleteLater())     );
    connect(t, SIGNAL(started()) , c, SLOT(onThreadStarted()) );

    connect(c, SIGNAL(destroyed(QObject*))                                              , this, SLOT(onOneDestFromQueueSha1LocalFs())                                   );
    connect(c, SIGNAL(appendMac2queueSyncRequest(QStringList,int))                      , this, SLOT(appendMac2queueSyncRequest(QStringList,int))                       );
    connect(c, SIGNAL(setLocalMacDateSha1(QStringList,QStringList,QList<QDateTime>,int)), this, SLOT(setLocalMacDateSha1(QStringList,QStringList,QList<QDateTime>,int)) );

    connect(c, SIGNAL(checkRemovedMacs(QStringList,int))                                , this, SIGNAL(checkRemovedMacs(QStringList,int))                               );
    t->start();

}
//---------------------------------------------------------------------------------------------------
void BackUpManager::removeMacFromQueueCheckLocalFs(const QString &mac)
{
    checkSha1LocalFsMacQueue.removeOne(mac);
}
//---------------------------------------------------------------------------------------------------
void BackUpManager::removeMacFromSyncQueue(const QString &mac)
{
    syncRequestQueue.removeOne(mac);
}
//---------------------------------------------------------------------------------------------------
QStringList BackUpManager::removeOldConnections(QHash<qint64, QStringList> &hDateConnected2macLst, QHash<QString, qint64> &hConnectedMac2date, quint32 currConnCounter,
                                         const quint32 &maxConnSize, QHash<QString, bool> &hAliveMacConn)
{
    QStringList killedMacs;
    //видаляю старі з'єднання, крім тих що використовуються
    QList<qint64> lk = hDateConnected2macLst.keys();
    std::sort(lk.begin(), lk.end());

    for(int i = 0; i < 10000 && !lk.isEmpty() && currConnCounter >= maxConnSize; i++){
        QStringList macL = hDateConnected2macLst.value(lk.at(i));
        QStringList l;
        for(int j = 0, jMax = macL.size(); j < jMax; j++){
            if(hAliveMacConn.contains(macL.at(j))){
                l.append(macL.at(j));
            }else{
                killedMacs.append(macL.at(j));
                currConnCounter--;
                hConnectedMac2date.remove(macL.at(j));
                hAliveMacConn.remove(macL.at(j));
            }
        }

        if(l.isEmpty())
            hDateConnected2macLst.remove(lk.at(i));
        else
            hDateConnected2macLst.insert(lk.at(i), l);
    }
    return killedMacs;
}

//---------------------------------------------------------------------------------------------------
