#ifndef M2MCONNHOLDERDECODER_H
#define M2MCONNHOLDERDECODER_H

///[!] matilda-bbb-shared
#include "src/protocolprocessors/decodematildaprotocolbase.h"

///[!] svaha-service
#include "m2m-service-src/main/m2mservertypes.h"



///[!] MatildaIO
#include "matilda-bbb-src/tcpspeedstat.h"
#include "src/shared/readwriteiodevice.h"


///[!] type-converter
#include "src/shared/networkconverthelper.h"


#include "m2m-server-define.h"

#include "connectiontableinsharedmemorytypes.h"

class M2MConnHolderDecoder : public DecodeMatildaProtocolBase
{
    Q_OBJECT
public:
    explicit M2MConnHolderDecoder(const QHostAddress &peerAddress, const qintptr &socketDescriptor, const bool &verboseMode, QObject *parent = nullptr);


    struct ConnHolderStateParams
    {
        qint8 connectedDevType;// isMatildaDev; //-1 unknown, 0 - Matilda-conf, 1 - matilda-dev

        //use connId instead
//        QString myIPstr; // peerAddress().toString();
//        QString mySocketID;//socketDescriptor())
//        QString myRemoteIpAndDescr;


        QStringList mMac; //my mac list
        QString mIden; //my Object ID

        //M2M server connection params
        //service - connection holder
//        QString serverServiceIP;
//        quint16 serverServicePort;//65000

        //service - data exchange
        QString serverDataIP;

        quint16 serverDataStart;
        quint16 serverDataEnd;


        quint8 backupSessionId;
        quint16 lastBackupServerPort;
        QString lastSha1base64;//backup hash
        QDateTime dtLastBackupCheck;
        QStringList macL4backupManager;//a list of MAC that have backups
        QString workDir;

        quint8 ucDeviceType;   //types like UC, EMUL0, EMUL1


        ConnHolderStateParams() : connectedDevType(REM_DEV_MATILDA_UNKNWN),
            serverDataStart(0), serverDataEnd(0),
            backupSessionId(0), lastBackupServerPort(0) , ucDeviceType(DEV_UNKNWN) {}
    } myStateParams;

//    bool useJsonMode; use lastObjSett

    QTime timeObjectSmpl;

    struct M2MZombieState
    {
        QTime timeZombie;
        bool isWaiting4answer; //this side is the last initiator or not
        quint32 zombieMsec;

        M2MZombieState() : isWaiting4answer(false),
            zombieMsec(300000) {}
    } myZombieKiller;
//    IneedMoreTimeObject *timeObject; matilda-bbb-serverside-shared


    static bool isCommandAllowed4thisConnectedDev(const quint16 &command, const qint8 &connectedDevType);

    static QList<quint16> getCommand4remDevMatilda();
    static QList<quint16> getCommand4remDevUCon();

    QString getRemoteIpAndDescr();

    QStringHash getObjIfo(const QVariantMap &h, const bool &addVersion);


    ConnectionTableInSharedMemoryConnection theconnection;

    bool areServerIpPortValidAndMySocketID(const QString &serverIp, const quint16 &serverPort, const QString &objSocketId);

    bool isMySocketID(const QString &objSocketId);

    bool isMySocketIDinTheList(const QStringList &objSocketIds);



    void decodeReadDataJSON(QByteArray dataArr);



    FunctionRezultJSON getFunctionRezult4aCommandJSON(const quint16 &command, const QVariantHash &hash, bool &doAfter);


    FunctionRezultJSON onCOMMAND_YOUR_ID_AND_MAC(const QVariantHash &hash, bool &doAfter);

    FunctionRezultJSON onCOMMAND_I_AM_A_ZOMBIE(const QVariantHash &hash);

    FunctionRezultJSON onCOMMAND_CONNECT_ME_2_THIS_ID_OR_MAC(const QVariantHash &hash, bool &doAfter);


    FunctionRezultJSON onCOMMAND_CHECK_BACKUP_FILE_HASH_SUMM(const QVariantHash &hash, bool &doAfter);


    quint16 startUploadBackup(const QString &lastSha1base64);

    QByteArray getBackupSign(const quint16 &serverPort);

    void onDoAfter(const quint16 &command);


    void createZombieTmr(const int &zombieMsec);

signals:
    //to the socket

    void closeTheConnection();

    void createBackupReceiver(QString workDir, quint16 startPort, quint16 endPort);


    //to the resource manager and shared memory
    void setServerInConnIdExtData(QString conntype, QString connid, qint64 msecstart, qint64 msecend, qint64 rb, qint64 wb, QString lastmessage); //список вхідних IP адрес (АйПі збиратора ігнорується)


    //BackUpManager
    void onSyncRequestRemoteSha1isEqual(QStringList macL);//на віддаленому пристрої ХЕШ сума файлу не змінилась, не чіпаю, тільки видаляю з черги wait4answerSyncQueue

    void onSyncFileDownloaded(QStringList macL, QString lastSha1base64, qint64 msecCreated);//на віддаленому пристрої ХЕШ сума файлу змінилась, завантаження здійснено

    void startTmrCheckRemoteSha1();


//to M2M service
    void addMyId2Hash(QString objId, QStringList macl, QString remIpDescr, QStringHash hashObjIfo, bool add2sync);//id mac <remote ip>:<descr>

    void removeMyId2Hash(QStringList idMacList, QString remIpDescr);//id is mac

    void removeThisIpFromTemporaryBlockList(QString ip);


//    void connMe2ThisIdOrMac(QString macOrId, bool isMacMode, QString socket, QString);//mac or id, isMacMode, socket id
    void connMe2ThisIdOrMac(QString macOrId, bool isMac, QString myRemoteId, QString rIp);//mac or id, isMacMode, socket id

    void showMessage(QString message);

    void setInfoAboutObj(QStringList macL, QStringHash objIfo, int counter);//infoAboutObj


    void onForceReading();


    void startTmrZombieKiller(int msec);


    void onM2MServerAcceptedMe();

    void killTemporaryObjects();


public slots:
    //from the connection holder server
    void startConnMatildaDev(quint16 serverPort, QString objId, QString objMac, QString objSocketId, QString rIp);//server: ip, port, socket id mac <remote ip>:<descr>

    void startConn4UCon(quint16 serverPort,  QString objSocketId);//server: ip, port, socket id mac <remote ip>:<descr>

//    void connMe2ThisIdOrMacSlot(QStringList macIdList, QString socketId);//<mac>@<id>
    void onFailed2connect2oneDev(QStringList macIdList, QString socketId);//<mac>@<id>

    void onResourBusy(QString socketId);

    void checkThisMac(QString mac);

//    void killClientNow(QString id, bool byDevId);
    void killClientsNow(QStringList ids, bool byDevId); //v11


    //BackUpManager
    void checkBackup4thisMac(QString mac, QString lastSha1base64);//check SHA1 for last backup file and if not equal: create new backup (check settings before this)  and upload to service

    void onSyncDone(quint8 backupSessionId, QStringList macL, QString lastSha1base64, qint64 msecCreated);//на віддаленому пристрої ХЕШ сума файлу змінилась, завантаження здійснено

//    void onSyncDone(quint8 sessionId, QStringList macL, QString lastSha1base64, QDateTime dtCreatedUtc);//на віддаленому пристрої ХЕШ сума файлу змінилась, завантаження здійснено

    void onSyncServiceDestr(quint8 backupSessionId);




    //from the connection holder socket
    void restartTimeObject();



    //to the resource manager and shared memory
    void sendLastConnectionState();



    void checkRemoteSha1();


    //from the parent
    void onDataConnectionParamsChanged(QString serverDataIP, quint16 serverDataStart, quint16 serverDataEnd);

    void onBackupWorkDirectoryChanged(QString workDir);

    void onEverythingIsConnected();


    //zombie checker
    void setZombieMsec(int msec);

    void checkSendZombieCommand();

    void restartZombieTmr();

    void restartZombieTmrExt(const bool &fastMode);

    void fastZombieCheckSmart();

    //connection is down
    void onConnectionIsDown();

    void startStopSuicideTmr(QString remIpDescr, bool start);

    void startSuicide();


};

#endif // M2MCONNHOLDERDECODER_H
