#include "m2mbackupsocket.h"

//-----------------------------------------------------------------------------------------------------------

M2MBackupSocket::M2MBackupSocket(QObject *parent) : QTcpSocket(parent)
{
    connect(this, &M2MBackupSocket::readyRead, this, &M2MBackupSocket::mReadyRead);
    connect(this, &M2MBackupSocket::disconnected, this ,&M2MBackupSocket::onDisconn);


}

//-----------------------------------------------------------------------------------------------------------

bool M2MBackupSocket::isConnOpen()
{
    const bool stopAll = !(state() == QAbstractSocket::ConnectedState || state() == QAbstractSocket::ConnectingState);
    return !stopAll;
}

//-----------------------------------------------------------------------------------------------------------

void M2MBackupSocket::onDisconn()
{
    emit iAmDisconn();
    close();
    deleteLater();
}

//-----------------------------------------------------------------------------------------------------------

void M2MBackupSocket::onDisconnByDecoder()
{
    onDisconn();
}

//-----------------------------------------------------------------------------------------------------------

void M2MBackupSocket::createDecoder(const bool &verboseMode, const QByteArray &write4authorizeBase64, const QString &lastSha1base64, const QString &workDir)
{
    decoder = new M2MBackupConnDecoder(verboseMode, this);


    connect(decoder, &M2MBackupConnDecoder::disconnLater  , this, &M2MBackupSocket::disconnLater);
    //    connect(decoder, &M2MBackupConnDecoder::createBackupReceiver, this, &M2MBackupConnDecoder::createBackupReceiver);

    connect(decoder, &M2MBackupConnDecoder::writeThisData       , this, &M2MBackupSocket::mWrite2Socket);
    connect(decoder, &M2MBackupConnDecoder::writeThisDataJSON   , this, &M2MBackupSocket::mWrite2SocketJSON);
    connect(decoder, &M2MBackupConnDecoder::addError2Log        , this, &M2MBackupSocket::addError2Log);

    connect(decoder, &M2MBackupConnDecoder::syncDone, this, &M2MBackupSocket::syncDone);

    connect(this, &M2MBackupSocket::restartTimeObject, decoder, &M2MBackupConnDecoder::restartTimeObject);

    decoder->setBackupParams( write4authorizeBase64, lastSha1base64, workDir);

    connect(decoder, &M2MBackupConnDecoder::onForceReading, this, &M2MBackupSocket::mReadyRead);

}

//-----------------------------------------------------------------------------------------------------------

void M2MBackupSocket::setTimeouts(const ConnectionTimeouts &socketTimeouts)
{
    this->socketTimeouts = socketTimeouts;
}

//-----------------------------------------------------------------------------------------------------------

void M2MBackupSocket::mWrite2SocketJSON(QJsonObject data, quint16 command)
{
    if(!isConnOpen())
        return;

    emit restartTimeObject();
    decoder->speedStat.lastByteWrt = ReadWriteIODevice::mWrite2SocketJSON(this,
                                                                          data, command,
                                                                          decoder->lastObjSett, decoder->speedStat.connSpeed);


    decoder->speedStat.totalBytesTransm += decoder->speedStat.lastByteWrt;
    decoder->speedStat.writeCounter++;

//    decoder->theconnection.wb += decoder->speedStat.lastByteWrt;
//    decoder->sendLastConnectionState();
}

//-----------------------------------------------------------------------------------------------------------

void M2MBackupSocket::mWrite2Socket(QVariant data, quint16 command)
{
    if(decoder->lastObjSett.verboseMode)
        qDebug() << "M2MBackupSocket::mWriteToSocket " << command;
    if(!isConnOpen())
        return;

    //QIODevice *device, const QVariant &s_data, const quint16 &s_command, LastMatildaObjSett &lastObjSett, const qint64 &connSpee
    //    QIODevice *f = new QIODevice();
    decoder->speedStat.lastByteWrt = ReadWriteIODevice::mWrite2Socket(this,
                                                                      data, command,
                                                                      decoder->lastObjSett, decoder->speedStat.connSpeed);//   //     qDebug() << block.toHex();
    decoder->speedStat.totalBytesTransm += decoder->speedStat.lastByteWrt;
    decoder->speedStat.writeCounter++;
    //    if(socketcache.verboseMode)
    if(decoder->lastObjSett.verboseMode)
        qDebug() << "M2MBackupSocket:: lastByteWrt= " << QTime::currentTime().toString("hh:mm:ss.zzz") << socketDescriptor()
                                         << decoder->speedStat.lastByteWrt << command << decoder->speedStat.totalBytesTransm << decoder->speedStat.writeCounter;

    emit restartTimeObject();

//    decoder->theconnection.wb += decoder->speedStat.lastByteWrt;
//    decoder->sendLastConnectionState();
}

//-----------------------------------------------------------------------------------------------------------

void M2MBackupSocket::disconnLater(qint64 msec)
{
    QTimer::singleShot(msec, this, SLOT(onDisconnByDecoder()));

}

//-----------------------------------------------------------------------------------------------------------

void M2MBackupSocket::mReadyRead()
{
    disconnect(this, &M2MBackupSocket::readyRead, this, &M2MBackupSocket::mReadyRead);
    readFunction();
    connect(this, &M2MBackupSocket::readyRead, this, &M2MBackupSocket::mReadyRead);
}

//-----------------------------------------------------------------------------------------------------------

void M2MBackupSocket::readFunction()
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
        const QList<QByteArray> readList = ReadWriteIODevice::readIODevice(this,
                                                                           socketTimeouts.timeOutBMsec, socketTimeouts.timeOutGMsec,
                                                                           decoder->lastObjSett.verboseMode, addLen, hasErr);

        decoder->speedStat.totalBytesRecvd += addLen;
//        decoder->theconnection.rb += addLen;
//        decoder->sendLastConnectionState();

        emit restartTimeObject();
        if(hasErr || readList.isEmpty()){
            qDebug()<< "M2MBackupSocket::readServer:"<< readList;
            decoder->speedStat.badByteReceived += addLen;
        }else{
            for(int i = 0, iMax = readList.size(); i < iMax; i++)
                decoder->decodeReadDataJSON(readList.at(i));
        }

        return;
    }

    bool hasErr, hasMoreData;
    quint16 serverCommand = COMMAND_ZULU;
    quint32 blockSize;
    const QVariant readVar = ReadWriteIODevice::readIODevice(this,
                                                             socketTimeouts.timeOutBMsec, socketTimeouts.timeOutGMsec,
                                                             hasErr, hasMoreData, serverCommand, blockSize);
    emit restartTimeObject();

    decoder->speedStat.totalBytesRecvd += blockSize;
//    decoder->theconnection.rb += blockSize;
//    decoder->sendLastConnectionState();

    if(hasErr){
        emit addError2Log(QString("broken packet. ip: %1,bytesAvailable: %2, blockSize: %3").arg(decoder->connId.peerAddress).arg(bytesAvailable()).arg(blockSize));
        qDebug()<< "M2MBackupSocket::readServer:" << blockSize << bytesAvailable() << readAll().toHex();

        decoder->lastDecodeAllowedCommand = 0;
        mWrite2Socket(decoder->onCommandErrorLastOperation(ERR_CORRUPTED_DATA).s_data, COMMAND_ERROR_CODE);
        decoder->speedStat.badByteReceived += bytesAvailable();
        return;
    }

    if(serverCommand > COMMAND_ZULU){
        qint64 lenuncompressed = -1;
        decoder->decodeReadData(
                    (serverCommand == COMMAND_COMPRESSED_PACKET) ?
                        ReadWriteIODevice::uncompressRead(readVar.toByteArray(), serverCommand, lenuncompressed, decoder->lastObjSett.verboseMode)
                      : readVar, serverCommand);

    }
    if(hasMoreData)
        QTimer::singleShot(11, this, SLOT(mReadyRead()) );
}

//-----------------------------------------------------------------------------------------------------------

