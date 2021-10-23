#include "m2mconnholdersocket.h"


///[!] svaha-service
#include "m2m-service-src/data-transmission/m2mbackupserver.h"




#include "matildalimits.h"


//-------------------------------------------------------------------------------------

M2MConnHolderSocket::M2MConnHolderSocket(QObject *parent) : QTcpSocket(parent)
{
    stopAll = false;
    decoder = 0;
}

//-------------------------------------------------------------------------------------

bool M2MConnHolderSocket::isConnOpen()
{
    stopAll = !(state() == QAbstractSocket::ConnectedState || state() == QAbstractSocket::ConnectingState);
    return !stopAll;
}

//-------------------------------------------------------------------------------------

void M2MConnHolderSocket::onThreadStartedVerb()
{
    onThreadStartedExt(true);
}

//-------------------------------------------------------------------------------------

void M2MConnHolderSocket::onThreadStarted()
{
    onThreadStartedExt(false);
}

//-------------------------------------------------------------------------------------

void M2MConnHolderSocket::createBackupReceiver(QString workDir, quint16 startPort, quint16 endPort)
{

    decoder->myStateParams.backupSessionId++;
    M2MBackupServer *server = new M2MBackupServer(decoder->lastObjSett.verboseMode, decoder->myStateParams.backupSessionId);

    const quint16 port = server->findFreePort(startPort, endPort);
    if(port == 0){
        server->deleteLater();;
//        emit add2
        return;
    }


    server->setWrite4aut(decoder->getBackupSign(port), decoder->myStateParams.lastSha1base64, workDir); //continue the process
    server->setZombieAndTimeoutMsec(socketTimeouts);


    QThread *thread = new QThread;

    thread->setObjectName("M2MConnHolderSocket");
    server->moveToThread(thread);

    connect(thread, &QThread::started, server, &M2MBackupServer::onThreadStarted);
    connect(server, SIGNAL(destroyed(QObject*)), thread, SLOT(quit()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()) );


    connect(decoder, &M2MConnHolderDecoder::startTmrCheckRemoteSha1, server, &M2MBackupServer::onZombie);//kill
    connect(server, &M2MBackupServer::onSyncDone, decoder, &M2MConnHolderDecoder::onSyncDone);
    connect(server, &M2MBackupServer::onSyncServiceDestr, decoder, &M2MConnHolderDecoder::onSyncServiceDestr);

    thread->start();

//    Service4uploadBackup *server = new Service4uploadBackup(verbouseMode, backupSessionId);

}

//-------------------------------------------------------------------------------------

void M2MConnHolderSocket::onTimeoutsChanged(ConnectionTimeouts socketTimeouts)
{
    this->socketTimeouts = socketTimeouts;

    emit setZombieMsec(socketTimeouts.zombieMsec);

}



//-------------------------------------------------------------------------------------

void M2MConnHolderSocket::onDisconn()
{
    onDisconnExt(true);

}

//-------------------------------------------------------------------------------------

void M2MConnHolderSocket::onDisconnByDecoder()
{
    onDisconnExt(false);

}

//-------------------------------------------------------------------------------------

void M2MConnHolderSocket::onDisconnExt(const bool &allowdecoder)
{
    Q_UNUSED(allowdecoder)
    close();
    deleteLater();
}

//-------------------------------------------------------------------------------------


void M2MConnHolderSocket::createDecoder(const bool &verboseMode)
{
    decoder = new M2MConnHolderDecoder(peerAddress(), socketDescriptor(), verboseMode, this);

    connect(decoder, &M2MConnHolderDecoder::closeTheConnection, this, &M2MConnHolderSocket::onDisconnByDecoder);
    connect(decoder, &M2MConnHolderDecoder::createBackupReceiver, this, &M2MConnHolderSocket::createBackupReceiver);

    connect(decoder, &M2MConnHolderDecoder::onForceReading, this, &M2MConnHolderSocket::mReadyRead);

    connect(decoder, &M2MConnHolderDecoder::writeThisDataJSON, this, &M2MConnHolderSocket::mWrite2SocketJSON);



    connect(decoder, &M2MConnHolderDecoder::disconnLater, [=](qint64 msec){
        QTimer::singleShot(msec, this, SLOT(onDisconnByDecoder()));
    });

    connect(this, &M2MConnHolderSocket::setZombieMsec, decoder, &M2MConnHolderDecoder::setZombieMsec)
;
    decoder->createZombieTmr(socketTimeouts.zombieMsec);




}

//-------------------------------------------------------------------------------------



void M2MConnHolderSocket::mReadyRead()
{
    //it must be callsed by decoder for the first time, than it will use events
    disconnect(this, &M2MConnHolderSocket::readyRead, this, &M2MConnHolderSocket::mReadyRead);
    readFunction();
    connect(this, &M2MConnHolderSocket::readyRead, this, &M2MConnHolderSocket::mReadyRead);
}

//-------------------------------------------------------------------------------------

void M2MConnHolderSocket::readFunction()
{
    if(!(state() == QAbstractSocket::ConnectedState ))
        return;

    if(bytesAvailable() < 1)
        return;

   if(!decoder->checkStartReading(this, decoder->timeObjectSmpl.elapsed())){
       return;
   }



    if(decoder->lastObjSett.useJsonMode){

        bool hasErr;
        qint64 addLen;
        const QList<QByteArray> readList = ReadWriteIODevice::readIODevice(
                    this,
                    socketTimeouts.timeOutBMsec, socketTimeouts.timeOutGMsec,
                    decoder->lastObjSett.verboseMode, addLen, hasErr);

        decoder->speedStat.totalBytesRecvd += addLen;
        decoder->theconnection.rb += addLen;
        decoder->sendLastConnectionState();

        emit restartTimeObject();
        if(hasErr || readList.isEmpty()){
            qDebug()<< "readServer:"<< readList;
            decoder->speedStat.badByteReceived += addLen;
        }else{
            for(int i = 0, iMax = readList.size(); i < iMax; i++)
                decoder->decodeReadDataJSON(readList.at(i));
        }

        return;
    }

//only JSON mode
    onDisconn();
    return;

}

//-------------------------------------------------------------------------------------

void M2MConnHolderSocket::mWrite2SocketJSON(QJsonObject jObj, const quint16 s_command)
{
    if(!isConnOpen())
        return;

    emit restartTimeObject();
    decoder->speedStat.lastByteWrt = ReadWriteIODevice::mWrite2SocketJSON(this, jObj, s_command, decoder->lastObjSett, decoder->speedStat.connSpeed);


    decoder->speedStat.totalBytesTransm += decoder->speedStat.lastByteWrt;
    decoder->speedStat.writeCounter++;

    decoder->theconnection.wb += decoder->speedStat.lastByteWrt;
    decoder->sendLastConnectionState();

}

//-------------------------------------------------------------------------------------

void M2MConnHolderSocket::onThreadStartedExt(const bool &verboseMode)
{
    createDecoder(verboseMode);

    emit onThisDecoderReady(decoder);

    connect(this, &M2MConnHolderSocket::disconnected, this, &M2MConnHolderSocket::onDisconn);
    if(state() == QAbstractSocket::ConnectedState ){
        QTimer::singleShot(socketTimeouts.msecAlive, this, SLOT(onDisconn()));//SETT_TIME_2_LIVE
        return;
    }

    QTimer::singleShot(11, this, SLOT(onDisconn()));


}

//-------------------------------------------------------------------------------------


