#ifndef BACKUPMANAGER_H
#define BACKUPMANAGER_H

#include <QObject>
#include <QtCore>

class BackUpManager : public QObject
{
    Q_OBJECT
public:
    explicit BackUpManager(QObject *parent = 0);

signals:
    void checkBackup4thisMac(QString mac, QByteArray lastSHA1);//check SHA1 for last backup file and if not equal: create new backup (check settings before this)  and upload to service

public slots:
    void onThreadStarted();


    void reloadSettings();//parallel count

    void onConnectedThisMacs(QStringList macL); //only if allowed sync

    void onDisconnectedThisMacs(QStringList macL);

    void onDisconnectedThisMac(QString mac);



    void onLastSHA1_4File(QStringList macL, QByteArray lastSHA1, QDateTime dtCreationUTC);//check or upload done




private:
    struct Mac2syncInfo{
        QDateTime dtLastFile;
        QDateTime dtLastCheck; //last client response about backup file
        QByteArray lastFileSha1;//last
    };

    QHash<QString,Mac2syncInfo> hMac2syncInfo;//last
    QStringList mac2hash;//keys 4 hMac2syncInfo

//    QHash<QString,QByteArray> hMac2lastFileSHA1;//
//    QHash<QString,QDateTime> hMac2dtLastCheck;//
    quint32 maxMacSize;//maximum Mac count
    quint32 curMacSize;//current Mac count

    QStringList checkMacQueue;//mac address need 2 check Sha1


};

#endif // BACKUPMANAGER_H
