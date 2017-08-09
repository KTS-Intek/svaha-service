#ifndef BACKUPMANAGER_H
#define BACKUPMANAGER_H

#include <QObject>
#include <QtCore>



class BackUpManager : public QObject
{
    Q_OBJECT
public:
    explicit BackUpManager(const bool &verboseMode, QObject *parent = 0);

signals:
    void checkBackup4thisMac(QString mac, QString lastSha1base64);//check SHA1 for last backup file and if not equal: create new backup (check settings before this)  and upload to service



    void tmrQueueStart(int msec);


    void checkRemovedMacs(QStringList macL, int counter);

    void setMaxSett(qint32 maxYearSave, qint32 maxMacsSave, qint32 minUniqNumb, QString workDir);



public slots:
    void onThreadStarted();


    void reloadSettings();//parallel count

    void onConnectedThisMacs(QStringList macL); //only if allowed sync, check mac, add 2 checkSha1LocalFsMacQueue

    void onDisconnectedThisMacs(QStringList macL, int counter);

    void onDisconnectedThisMac(QString mac);




    void setLocalMacDateSha1(QStringList macL, QStringList sha1L, QList<QDateTime> dtCreatedUtcL, int counter);

    void appendMac2queueSyncRequest(QStringList macL, int counter);//no file or file too old


    void onCheckQueueSha1LocalFs();

    void onOneDestFromQueueSha1LocalFs();


    void onSyncRequestRemoteSha1isEqual(QStringList macL);//на віддаленому пристрої ХЕШ сума файлу не змінилась, не чіпаю, тільки видаляю з черги wait4answerSyncQueue

    void onSyncFileDownloaded(QStringList macL, QString lastSha1base64, QDateTime dtCreatedUtc);//на віддаленому пристрої ХЕШ сума файлу змінилась, завантаження здійснено



private:
    QDateTime dt4check(QDateTime &currDtUtc);

    bool isNeed2syncRequest(const QString &mac, const QDateTime &dt, const QDateTime &currDtUtc, QString &lastSha1);

    void startCheckMacGroup(const QStringList &macL);

    void removeMacFromQueueCheckLocalFs(const QString &mac);

    void removeMacFromSyncQueue(const QString &mac);

    QStringList removeOldConnections(QHash<qint64, QStringList> &hDateConnected2macLst, QHash<QString,qint64> &hConnectedMac2date, quint32 currConnCounter,
                              const quint32 &maxConnSize, QHash<QString, bool> &hAliveMacConn);

    struct Mac2syncInfo{
        QDateTime dtLastSyncFile; //дата створення файлу резервної копії в локальній ФС
        QDateTime dtSyncRequest; //фіксую час запиту, на виконання запиту відводжу 12 годин, після виконання запиту обнуляю час
        QString lastSha1base64;//last
    };

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

    } syncSett;


    QHash<qint64, QStringList> hDateConnected2macLst; // містить в собі пару дата підключення (currentMSecsSinceEpoch) МАК, при переповненні видаляю старі підключення, відповідно видаляю їх з інших контейнерів
    QHash<QString,qint64> hConnectedMac2date;//те саме що і hDateConnected2macLst, але ключ і значення поміняно місцями

    QHash<QString,bool> hAliveMacConn;//містить лише під'єднані МАК адреси

    QHash<QString,Mac2syncInfo> hMac2syncInfo;//містить МАК, резервні копії яких є перевіреними в локальній ФС, не може містити більше ключів ніж у hConnectedMac2date
    QStringList listKeys4hMac2syncInfo;//послідовний порядок ключів до hMac2syncInfo,наповнюється по мірі під'єднання

    QStringList syncRequestQueue;//послідовний порядок ключів до hMac2syncInfo,наповнюється з listKeys4hMac2syncInfo коли стає порожнім, зменшується по мірі від'єднання та по запитам на синхронізацію
    QHash<QString,bool> wait4answerSyncQueue;//очистка: отримано відповідь або підключення/відключення, додавання тільки перед відправкою запиту




//    QHash<QString,QByteArray> hMac2lastFileSHA1;//
//    QHash<QString,QDateTime> hMac2dtLastCheck;//


    QDateTime dtLastCheck;
    QStringList checkSha1LocalFsMacQueue;//mac address need 2 check Sha1, не може бути більшим за hConnectedMac2date

    quint32 currSha1LocalFsMacQueue;//current threads for check sha1


    bool verboseMode;


};

#endif // BACKUPMANAGER_H
