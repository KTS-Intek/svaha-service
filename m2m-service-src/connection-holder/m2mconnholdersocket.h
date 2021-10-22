#ifndef M2MCONNHOLDERSOCKET_H
#define M2MCONNHOLDERSOCKET_H

#include <QTcpSocket>


///[!] svaha-service
#include "m2m-service-src/connection-holder/m2mconnholderdecoder.h"


///[!] MatildaIO
#include "matilda-bbb-src/tcpspeedstat.h"


/*
 * Є два режими
 * 1. Від присторю опитування - при з’єданні очікує на отримання ІД та МАК адреси
 * 2. Від ПЗ конфігурації - при з’єднанні очікує на отримання ІД або МАК адреси клієнта з яким необхідно утворити пару
 *
 *
 *
 */



class M2MConnHolderSocket : public QTcpSocket
{
    Q_OBJECT
public:
    explicit M2MConnHolderSocket(QObject *parent = nullptr);

    bool stopAll;


    M2MConnHolderDecoder *decoder;

    ConnectionTimeouts socketTimeouts;//socketcache

    bool isConnOpen();



signals:


    //to decoder
    void restartTimeObject();

    void onThisDecoderReady(M2MConnHolderDecoder *decoder);



public slots:
    void onThreadStartedVerb();
    void onThreadStarted();



    void createBackupReceiver(QString workDir, quint16 startPort, quint16 endPort);

    void onTimeoutsChanged(ConnectionTimeouts socketTimeouts);

    void setTimeouts(const int &timeoutGMsec, const int &timeoutBMsec);


    void onDisconn();

    void onDisconnByDecoder();

    void onDisconnExt(const bool &allowdecoder);

    void createDecoder(const bool &verboseMode);

private slots:
    void mReadyRead();

    void readFunction();


    void mWrite2SocketJSON(QJsonObject jObj, const quint16 s_command);






private:
    void onThreadStartedExt(const bool &verboseMode);



};

#endif // M2MCONNHOLDERSOCKET_H
