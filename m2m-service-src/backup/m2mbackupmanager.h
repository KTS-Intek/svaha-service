#ifndef M2MBACKUPMANAGER_H
#define M2MBACKUPMANAGER_H

#include <QObject>

#include "m2m-service-src/main/m2mservertypes.h"


#include <QDateTime>


//former BackUpManager

class M2MBackupManager : public QObject
{
    Q_OBJECT
public:
    explicit M2MBackupManager(const bool &verboseMode, QObject *parent = nullptr);

    struct Mac2syncInfo{
        qint64 msecLastSyncFile; //дата створення файлу резервної копії в локальній ФС
        qint64 msecSyncRequest; //фіксую час запиту, на виконання запиту відводжу 12 годин, після виконання запиту обнуляю час
        QString lastSha1base64;//last
        Mac2syncInfo() : msecLastSyncFile(0), msecSyncRequest(0) {}
    };

    struct BackupManagerParams
    {
        bool verboseMode;


        QHash<qint64, QStringList> hDateConnected2macLst; // містить в собі пару дата підключення (currentMSecsSinceEpoch) МАК, при переповненні видаляю старі підключення, відповідно видаляю їх з інших контейнерів
        QHash<QString,qint64> hConnectedMac2date;//те саме що і hDateConnected2macLst, але ключ і значення поміняно місцями

        QHash<QString,bool> hAliveMacConn;//містить лише під'єднані МАК адреси

        QHash<QString,Mac2syncInfo> hMac2syncInfo;//містить МАК, резервні копії яких є перевіреними в локальній ФС, не може містити більше ключів ніж у hConnectedMac2date
        QStringList listKeys4hMac2syncInfo;//послідовний порядок ключів до hMac2syncInfo,наповнюється по мірі під'єднання

        QStringList syncRequestQueue;//послідовний порядок ключів до hMac2syncInfo,наповнюється з listKeys4hMac2syncInfo коли стає порожнім, зменшується по мірі від'єднання та по запитам на синхронізацію
        QHash<QString,bool> wait4answerSyncQueue;//очистка: отримано відповідь або підключення/відключення, додавання тільки перед відправкою запиту



        qint64 msecLastCheck;// dtLastCheck;
        QStringList checkSha1LocalFsMacQueue;//mac address need 2 check Sha1, не може бути більшим за hConnectedMac2date

        quint32 currSha1LocalFsMacQueue;//current threads for check sha1


        BackupManagerParams() : verboseMode(false), msecLastCheck(0), currSha1LocalFsMacQueue(0) {}
    } myParams;

    SyncServerSett syncParams;


    qint64 msec4check(qint64 &currMsec); //dt4check

    //former isNeed2syncRequest , my english were almost perfect )
    bool isItTime2makeASyncRequest(const QString &mac, const qint64 &msec, const qint64 &currMsec, QString &lastSha1);


    QStringList removeOldConnections(QHash<qint64, QStringList> &hDateConnected2macLst, QHash<QString,qint64> &hConnectedMac2date, quint32 currConnCounter,
                              const quint32 &maxConnSize, QHash<QString, bool> &hAliveMacConn);
signals:
    //to parent
    void gimmeSettings();


    void checkBackup4thisMac(QString mac, QString lastSha1base64);//check SHA1 for last backup file and if not equal: create new backup (check settings before this)  and upload to service


    //to killer
    void checkRemovedMacs(QStringList macL, int counter);

    void setMaxSett(qint32 maxYearSave, qint32 maxMacsSave, qint32 minUniqNumb, QString workDir);



//local
    void tmrQueueStart(int msec);

    void kickOffAll();


public slots:
    void onThreadStarted();

    void createOldBackupKiller();

    void stopAllAndDie();

    void setSyncParams(QString workDir, quint8 syncMode, quint8 maxFileCount, quint32 maxSizeMacTable, quint32 maxCountSha1LocalFsParallel, qint32 maxSizeSyncRequest, quint32 maxCountSyncRequestParallel, qint32 maxYearSave, qint32 minUniqMacs);


    void onOneDestFromQueueSha1LocalFs();

    void setLocalMacDateSha1(QStringList macL, QStringList sha1L, QList<qint64> msecCreatedL, int counter);

    void appendMac2queueSyncRequest(QStringList macL, int counter);//no file or file too old



    void onConnectedTheseMacs(QStringList macL); //only if allowed sync, check mac, add 2 checkSha1LocalFsMacQueue

    void onDisconnectedTheseMacs(QStringList macL, int counter);

    void onDisconnectedThisMac(QString mac);




    void onSyncRequestRemoteSha1isEqual(QStringList macL);//на віддаленому пристрої ХЕШ сума файлу не змінилась, не чіпаю, тільки видаляю з черги wait4answerSyncQueue

    void onSyncFileDownloaded(QStringList macL, QString lastSha1base64, qint64 msecCreatedUtc);//на віддаленому пристрої ХЕШ сума файлу змінилась, завантаження здійснено



private slots:
    void onCheckQueueSha1LocalFs();


private:
    bool checkQueueSha1LocalFs();

    void startCheckMacGroupSmart(QStringList &macL);

    void startCheckMacGroup(const QStringList &macL);


};

#endif // M2MBACKUPMANAGER_H
