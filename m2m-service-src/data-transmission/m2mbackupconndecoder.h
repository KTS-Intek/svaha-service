#ifndef M2MBACKUPCONNDECODER_H
#define M2MBACKUPCONNDECODER_H

///[!] matilda-bbb-shared
#include "src/protocolprocessors/decodematildaprotocolbase.h"

///[!] svaha-service
#include "m2m-service-src/main/m2mservertypes.h"


#include "m2m-server-define.h"

class M2MBackupConnDecoder : public DecodeMatildaProtocolBase
{
    Q_OBJECT
public:
    explicit M2MBackupConnDecoder(const bool &verboseMode, QObject *parent = nullptr);

    struct BackupConnDecoderParams
    {
        QByteArray auth;
        QString lastSha1Base64;
        qint64 msecCreated;

        QString workDir;

        QVariantHash hashAboutObj; //it is used for file names
        QByteArray backupArr;
        int backupArrLen;

        BackupConnDecoderParams() {}
    } myParams;

    QTime timeObjectSmpl;



    static bool isCommandAllowed4thisConnectedDev(const quint16 &command, const bool &isQDSMode, const quint8 &accessLevel);


    void setBackupParams(QByteArray write4authorizeBase64, QString lastSha1base64, QString workDir);



    void decodeReadDataJSON(QByteArray dataArr);

    void decodeReadData(const QVariant &dataVar, const quint16 &command);




    FunctionRezultJSON getFunctionRezult4aCommandJSON(const quint16 &command, const QVariantHash &hash, bool &doAfter);

    FunctionRezultJSON onCOMMAND_ZULU(const QVariantHash &hash, bool &doAfter);

    FunctionRezult getFunctionRezult4aCommand(const quint16 &command, const QVariant &dataVar, bool &doAfter);

    FunctionRezult onCOMMAND_AUTHORIZE(const QVariant &dataVar, bool &doAfter);

    FunctionRezult onCOMMAND_I_AM_A_ZOMBIE();

    FunctionRezult onCOMMAND_GET_CACHED_BACKUP(const QVariant &dataVar, bool &doAfter);


    void saveBackupArrAsFile();

    QString fileNameFromAboutObject(QStringList &macL, const int &shaLen);

    void onDoAfter(const quint16 &command);

signals:
    void syncDone(QStringList macL, QString lastSha1base64, qint64 msecCreated);//на віддаленому пристрої ХЕШ сума файлу змінилась, завантаження здійснено

    void onForceReading();

public slots:
    //from socket
    void restartTimeObject();


};

#endif // M2MBACKUPCONNDECODER_H
