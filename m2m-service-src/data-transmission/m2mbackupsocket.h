#ifndef M2MBACKUPSOCKET_H
#define M2MBACKUPSOCKET_H

#include <QTcpSocket>


///[!] svaha-service
#include "m2m-service-src/data-transmission/m2mbackupconndecoder.h"


//former Socket4uploadBackup
class M2MBackupSocket : public QTcpSocket
{
    Q_OBJECT
public:
    explicit M2MBackupSocket(QObject *parent = nullptr);

    M2MBackupConnDecoder *decoder;

    ConnectionTimeouts socketTimeouts;//socketcache

    bool isConnOpen();

signals:

    void mReadData();

    void iAmDisconn();

    void syncDone(QStringList macL, QString lastSha1base64, qint64 msecCreated);//на віддаленому пристрої ХЕШ сума файлу змінилась, завантаження здійснено

    void addError2Log(QString message);

//to decoder
    void restartTimeObject();

public slots:


    void onDisconn();

    void onDisconnByDecoder();

    void createDecoder(const bool &verboseMode, const QByteArray &write4authorizeBase64, const QString &lastSha1base64, const QString &workDir);

    void setTimeouts(const ConnectionTimeouts &socketTimeouts);


    void mWrite2SocketJSON(QJsonObject data, quint16 command);

    void mWrite2Socket(QVariant data, quint16 command);


//    void addError2Log(QString message);

    void disconnLater(qint64 msec);

private slots:
    void mReadyRead();

    void readFunction();

private:


};

#endif // M2MBACKUPSOCKET_H
