#ifndef M2MBACKUPSERVER_H
#define M2MBACKUPSERVER_H

#include <QTcpServer>



#include "tcpspeedstat.h"

class M2MBackupServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit M2MBackupServer(bool verboseMode, quint8 backupSessionId, QObject *parent = nullptr);

    ~M2MBackupServer();

    struct BackupServerParams
    {
        bool verboseMode;
        quint8 backupSessionId;
        QByteArray auth;
        QString lastSha1Base64;
        QString workDir;
        qint16 connCounter;

        ConnectionTimeouts socketTimeouts;//socketcache


        BackupServerParams() : verboseMode(false), backupSessionId(0), connCounter(0)  {}

    } myParams;


    quint16 findFreePort(const quint16 &minPort, const quint16 &maxPort);

    void setWrite4aut(const QByteArray &auth, const QString &lastSha1Base64, const QString &workDir);

    void setZombieAndTimeoutMsec(const ConnectionTimeouts &socketTimeouts);


signals:
    void onSyncServiceDestr(quint8 backupSessionId);

    void dataFromRemote();

    void stopAllNow();

    void onSyncDone(quint8 backupSessionId, QStringList macL, QString lastSha1base64, qint64 msecCreated);//на віддаленому пристрої ХЕШ сума файлу змінилась, завантаження здійснено

protected:
    void incomingConnection(qintptr handle);



public slots:
    void onThreadStarted();

    void onOneDisconn();
    void onZombie();

    void onDestrSignl();

    void syncDone(QStringList macL, QString lastSha1base64, qint64 msecCreated);//на віддаленому пристрої ХЕШ сума файлу змінилась, завантаження здійснено
};

#endif // M2MBACKUPSERVER_H
