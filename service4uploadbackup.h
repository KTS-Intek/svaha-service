#ifndef SERVICE4UPLOADBACKUP_H
#define SERVICE4UPLOADBACKUP_H

#include <QTcpServer>
#include <QObject>
#include <QDateTime>
#include <QList>


class Service4uploadBackup : public QTcpServer
{
    Q_OBJECT
public:
    explicit Service4uploadBackup(bool verboseMode, quint8 backupSessionId, QObject *parent = 0);

    quint16 findFreePort(quint16 minP, const quint16 &maxP);

    void setWrite4aut(QByteArray a, QString lastSha1Base64);

signals:

    void dataFromRemote();

    void stopAllNow();

    void onSyncServiceDestr(quint8 sessionId);


    void onSyncDone(quint8 sessionId, QString lastSha1base64, QDateTime dtCreatedUtc);//на віддаленому пристрої ХЕШ сума файлу змінилась, завантаження здійснено


protected:
    void incomingConnection(qintptr handle);

public slots:
    void onThrdStarted();

    void onOneDisconn();
    void onZombie();

    void onDestrSignl();

    void syncDone(QString lastSha1base64, QDateTime dtCreatedUtc);//на віддаленому пристрої ХЕШ сума файлу змінилась, завантаження здійснено


private:
    int connCounter;
    bool verboseMode;
    quint8 backupSessionId;

    QByteArray write4auth;
    QString lastSha1Base64;



};

#endif // SERVICE4UPLOADBACKUP_H
