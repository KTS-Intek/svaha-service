/****************************************************************************
**
**   Copyright © 2016-2017 The KTS-INTEK Ltd.
**   Contact: http://www.kts-intek.com.ua
**
**  This file is part of svaha-service.
**
**  svaha-service is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  svaha-service is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with svaha-service.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/
#include "socketdlyatrymacha.h"
#include <QTimer>
#include <QHostAddress>
#include <QtCore>
#include <QStringList>
#include <QHostAddress>

#include <QJsonObject>
#include <QVariantMap>

#include "../matilda-bbb/moji_defy.h"
#include "settloader4svaha.h"
#include "readjsonhelper.h"
#include "service4uploadbackup.h"

#define REM_DEV_MATILDA_UNKNWN  -1
#define REM_DEV_MATILDA_DEV     0
#define REM_DEV_MATILDA_CONF    1

//----------------------------------------------------------------------------------------------------------------------------
SocketDlyaTrymacha::SocketDlyaTrymacha(const bool &verbouseMode, QObject *parent) : QTcpSocket(parent)
{    
    this->verbouseMode = verbouseMode;
    timeZombie.start();
    allowCompress = false;
    lastHashSumm = 1;
    isMatildaDev = REM_DEV_MATILDA_UNKNWN;
    stopAll = false;

}
//----------------------------------------------------------------------------------------------------------------------------
void SocketDlyaTrymacha::onThrdStarted()
{
    connect(this, SIGNAL(readyRead()), this, SLOT(mReadyRead()) );
    connect(this, SIGNAL(disconnected()), this, SLOT(onDisconn()) );

    SettLoader4svaha sLoader;

    QTimer *zombieTmr = new QTimer(this);
    zombieTmr->setSingleShot(true);
    zombieTmr->setInterval(sLoader.loadOneSett(SETT_ZOMBIE_MSEC).toInt());// 15 * 60 * 1000 );
    connect(zombieTmr, SIGNAL(timeout()), this, SLOT(onDisconn()) );
    connect(this, SIGNAL(disconnected()) , zombieTmr, SLOT(stop()) );
    connect(this, SIGNAL(readyRead()), zombieTmr, SLOT(start()) );
    zombieTmr->start();

    QTimer *killTmr = new QTimer(this);
    killTmr->setSingleShot(true);
    killTmr->setInterval(sLoader.loadOneSett(SETT_TIME_2_LIVE).toInt());// 24 * 60 * 60 * 1000);

    connect(killTmr, SIGNAL(timeout()) , this, SLOT(onDisconn()) );//максимальна тривалість 24 години
    connect(this, SIGNAL(disconnected()), killTmr, SLOT(stop()) );


}
//----------------------------------------------------------------------------------------------------------------------------
void SocketDlyaTrymacha::startConn(QString serverIp, int serverPort, QString objId, QString objMac, QString objSocketId, QString rIp)
{

    if(isMatildaDev == REM_DEV_MATILDA_DEV){
        if(objId == mIden && mMac.contains(objMac) && objSocketId == myRemoteIpAndDescr && !serverIp.isEmpty() && serverPort > 1000 && serverPort < 65535){
            QJsonObject jObj;
            jObj.insert("sIp", serverIp);
            jObj.insert("sP", serverPort);
            jObj.insert("rIp", rIp);
            mWrite2SocketJSON(jObj, COMMAND_CONNECT_2_THIS_SERVICE);

        }
    }

}
//----------------------------------------------------------------------------------------------------------------------------
void SocketDlyaTrymacha::startConn(QString serverIp, int serverPort, QString objSocketId)
{
    if(isMatildaDev == REM_DEV_MATILDA_CONF){
        if(objSocketId == myRemoteIpAndDescr && !serverIp.isEmpty() && serverPort > 1000 && serverPort < 65535){
            QJsonObject jObj;
            jObj.insert("sIp", serverIp);
            jObj.insert("sP", serverPort);
            mWrite2SocketJSON(jObj, COMMAND_CONNECT_ME_2_THIS_ID_OR_MAC);
        }
    }
}
//----------------------------------------------------------------------------------------------------------------------------
void SocketDlyaTrymacha::connMe2ThisIdOrMacSlot(QStringList macIdList, QString socketId)
{
    if(socketId == myRemoteIpAndDescr){
        QJsonObject jObj;
        if(macIdList.isEmpty()){
            jObj.insert("lcmd", COMMAND_CONNECT_ME_2_THIS_ID_OR_MAC);
            jObj.insert("e", ERR_RESOURCE_BUSY);
            jObj.insert("em", tr("Unknown device"));
            mWrite2SocketJSON(jObj, COMMAND_ERROR_CODE_EXT);

        }else{
            jObj.insert(QString("l"), arrFromList(macIdList));
            mWrite2SocketJSON(jObj, COMMAND_CONNECT_ME_2_THIS_ID_OR_MAC);
        }
//        jObj.insert("sIp", "");
//        jObj.insert("sp", "");

    }
}
//----------------------------------------------------------------------------------------------------------------------------
void SocketDlyaTrymacha::onResourBusy(QString socketId)
{
    if(socketId == myRemoteIpAndDescr){
       mWrite2SocketJSON(errCodeLastOperationJson(lastCommand, ERR_RESOURCE_BUSY), COMMAND_ERROR_CODE);//
    }
}
//----------------------------------------------------------------------------------------------------------------------------
void SocketDlyaTrymacha::checkThisMac(QString mac)
{
    if(stopAll)
        return;

    if(isMatildaDev == REM_DEV_MATILDA_DEV && mMac.contains(mac.toUpper())){
//        emit addMyId2Hash(mIden, QStringList() << mac, myRemoteIpAndDescr);
        onDisconn();
    }
}
//----------------------------------------------------------------------------------------------------------------------------
void SocketDlyaTrymacha::killClientNow(QString id, bool byDevId)
{
    if((byDevId && id == mIden) || (!byDevId && id == myRemoteIpAndDescr)){
        onDisconn();
    }
}
//----------------------------------------------------------------------------------------------------------------------------
void SocketDlyaTrymacha::checkBackup4thisMac(QString mac, QString lastSha1base64)
{
    if(mMac.contains(mac) && !macL4backupManager.contains(mac)){

        QDateTime dt = QDateTime::currentDateTimeUtc();
        if(!dtLastBackupCheck.isValid()){
            dtLastBackupCheck = dt;
            QTimer *t = new QTimer(this);
            t->setSingleShot(true);
            t->setInterval(111);

            connect(this, SIGNAL(startTmrCheckRemoteSha1()), t, SLOT(start()) );
            connect(this, SIGNAL(disconnected()), t, SLOT(stop()) );
            connect(t, SIGNAL(timeout()), this, SLOT(checkRemoteSha1()) );

        }else{
            if(dtLastBackupCheck > dt){
                //зараз виконуються операції з вивантаження файлу
                macL4backupManager.append(mac);
                return;
            }
//            if(dtLastBackupCheck <= dt)
                dtLastBackupCheck = dt;
//            if(dtLastBackupCheck.secsTo(dt) < 3600)
//                return;
        }

        this->lastSha1base64 = lastSha1base64;
        macL4backupManager.append(mac);


        emit startTmrCheckRemoteSha1();

        if(verbouseMode)
            qDebug() << "checkBackup4thisMac " << mac << lastSha1base64 << dtLastBackupCheck << dt;
    }
}
//----------------------------------------------------------------------------------------------------------------------------
void SocketDlyaTrymacha::onSyncDone(quint8 sessionId, QStringList macL, QString lastSha1base64, QDateTime dtCreatedUtc)
{
    if(verbouseMode)
        qDebug() << "SocketDlyaTrymacha::onSyncDone " << sessionId << backupSessionId << lastSha1base64 << dtCreatedUtc << this->lastSha1base64 << macL4backupManager;
    if(sessionId == backupSessionId) {
        macL4backupManager.append(macL);
        macL4backupManager.removeDuplicates();
        emit onSyncFileDownloaded(macL4backupManager, lastSha1base64, dtCreatedUtc);
        this->lastSha1base64 = lastSha1base64;
        macL4backupManager.clear();
        dtLastBackupCheck = QDateTime::currentDateTimeUtc();
    }
}
//----------------------------------------------------------------------------------------------------------------------------
void SocketDlyaTrymacha::onSyncServiceDestr(quint8 sessionId)
{
    if(verbouseMode)
        qDebug() << "SocketDlyaTrymacha::onSyncServiceDestr " << sessionId << backupSessionId << lastSha1base64 << macL4backupManager;

    if(sessionId == backupSessionId) {
        if(!macL4backupManager.isEmpty())
            emit onSyncRequestRemoteSha1isEqual(macL4backupManager);
        macL4backupManager.clear();
        dtLastBackupCheck = QDateTime::currentDateTimeUtc();
    }
}
//----------------------------------------------------------------------------------------------------------------------------
void SocketDlyaTrymacha::mReadyRead()
{
    disconnect(this, SIGNAL(readyRead()), this, SLOT(mReadyRead()) );
    mReadyReadF();
    connect(this, SIGNAL(readyRead()), this, SLOT(mReadyRead()) );
}

//----------------------------------------------------------------------------------------------------------------------------
void SocketDlyaTrymacha::mWrite2SocketJSON(QJsonObject jObj, const quint16 s_command)
{
    if(!isConnOpen())
        return;

    timeZombie.restart();
    jObj.insert("cmd", s_command);


    QByteArray writeArr ;
    if(true){
        QJsonDocument jDoc2DST(jObj);
        writeArr = jDoc2DST.toJson(QJsonDocument::Compact);
        writeArr.chop(1);// remove }
    }

//    qDebug() << "writeArr0 " << peerAddress() << peerPort() << writeArr;
    switch(lastHashSumm){
    case 0: { writeArr.append(", \"Md4\":\""      + QCryptographicHash::hash( writeArr + ", \"Md4\":\"0\"}"     , QCryptographicHash::Md4     ).toBase64() + "\"}" ); break;}
    case 2: { writeArr.append(", \"Sha1\":\""     + QCryptographicHash::hash( writeArr + ", \"Sha1\":\"0\"}"    , QCryptographicHash::Sha1    ).toBase64() + "\"}" ); break;}
    case 3: { writeArr.append(", \"Sha224\":\""   + QCryptographicHash::hash( writeArr + ", \"Sha224\":\"0\"}"  , QCryptographicHash::Sha224  ).toBase64() + "\"}" ); break;}
    case 4: { writeArr.append(", \"Sha256\":\""   + QCryptographicHash::hash( writeArr + ", \"Sha256\":\"0\"}"  , QCryptographicHash::Sha256  ).toBase64() + "\"}" ); break;}
    case 5: { writeArr.append(", \"Sha384\":\""   + QCryptographicHash::hash( writeArr + ", \"Sha384\":\"0\"}"  , QCryptographicHash::Sha384  ).toBase64() + "\"}" ); break;}
    case 6: { writeArr.append(", \"Sha512\":\""   + QCryptographicHash::hash( writeArr + ", \"Sha512\":\"0\"}"  , QCryptographicHash::Sha512  ).toBase64() + "\"}" ); break;}
    case 7: { writeArr.append(", \"Sha3_224\":\"" + QCryptographicHash::hash( writeArr + ", \"Sha3_224\":\"0\"}", QCryptographicHash::Sha3_224).toBase64() + "\"}" ); break;}
    case 8: { writeArr.append(", \"Sha3_256\":\"" + QCryptographicHash::hash( writeArr + ", \"Sha3_256\":\"0\"}", QCryptographicHash::Sha3_256).toBase64() + "\"}" ); break;}
    case 9: { writeArr.append(", \"Sha3_384\":\"" + QCryptographicHash::hash( writeArr + ", \"Sha3_384\":\"0\"}", QCryptographicHash::Sha3_384).toBase64() + "\"}" ); break;}
    case 10:{ writeArr.append(", \"Sha3_512\":\"" + QCryptographicHash::hash( writeArr + ", \"Sha3_512\":\"0\"}", QCryptographicHash::Sha3_512).toBase64() + "\"}" ); break;}
    default:{ writeArr.append(", \"Md5\":\""      + QCryptographicHash::hash( writeArr + ", \"Md5\":\"0\"}"     , QCryptographicHash::Md5     ).toBase64() + "\"}" ); break;}
    }

    qint64 blSize = writeArr.length();


    if(blSize >= SETT_MAX_UNCOMPRSS_PACkET_SIZE && allowCompress){


        if(true){

            QJsonObject jObjCpmrr;

            jObjCpmrr.insert("cmd", QString::number(COMMAND_COMPRESSED_PACKET));
            jObjCpmrr.insert("zlib", QString(qCompress(writeArr, 9).toBase64()));

            QJsonDocument jDoc2DST(jObjCpmrr);
            writeArr = jDoc2DST.toJson(QJsonDocument::Compact);
            writeArr.chop(1);// remove }
        }
        if(verbouseMode)
            qDebug() << "writeArr1 comprs " << peerAddress() << peerPort() << writeArr;
        switch(lastHashSumm){
        case 0: { writeArr.append(", \"Md4\":\""      + QCryptographicHash::hash( writeArr + ", \"Md4\":\"0\"}"     , QCryptographicHash::Md4     ).toBase64() + "\"}" ); break;}
        case 2: { writeArr.append(", \"Sha1\":\""     + QCryptographicHash::hash( writeArr + ", \"Sha1\":\"0\"}"    , QCryptographicHash::Sha1    ).toBase64() + "\"}" ); break;}
        case 3: { writeArr.append(", \"Sha224\":\""   + QCryptographicHash::hash( writeArr + ", \"Sha224\":\"0\"}"  , QCryptographicHash::Sha224  ).toBase64() + "\"}" ); break;}
        case 4: { writeArr.append(", \"Sha256\":\""   + QCryptographicHash::hash( writeArr + ", \"Sha256\":\"0\"}"  , QCryptographicHash::Sha256  ).toBase64() + "\"}" ); break;}
        case 5: { writeArr.append(", \"Sha384\":\""   + QCryptographicHash::hash( writeArr + ", \"Sha384\":\"0\"}"  , QCryptographicHash::Sha384  ).toBase64() + "\"}" ); break;}
        case 6: { writeArr.append(", \"Sha512\":\""   + QCryptographicHash::hash( writeArr + ", \"Sha512\":\"0\"}"  , QCryptographicHash::Sha512  ).toBase64() + "\"}" ); break;}
        case 7: { writeArr.append(", \"Sha3_224\":\"" + QCryptographicHash::hash( writeArr + ", \"Sha3_224\":\"0\"}", QCryptographicHash::Sha3_224).toBase64() + "\"}" ); break;}
        case 8: { writeArr.append(", \"Sha3_256\":\"" + QCryptographicHash::hash( writeArr + ", \"Sha3_256\":\"0\"}", QCryptographicHash::Sha3_256).toBase64() + "\"}" ); break;}
        case 9: { writeArr.append(", \"Sha3_384\":\"" + QCryptographicHash::hash( writeArr + ", \"Sha3_384\":\"0\"}", QCryptographicHash::Sha3_384).toBase64() + "\"}" ); break;}
        case 10:{ writeArr.append(", \"Sha3_512\":\"" + QCryptographicHash::hash( writeArr + ", \"Sha3_512\":\"0\"}", QCryptographicHash::Sha3_512).toBase64() + "\"}" ); break;}
        default:{ writeArr.append(", \"Md5\":\""      + QCryptographicHash::hash( writeArr + ", \"Md5\":\"0\"}"     , QCryptographicHash::Md5     ).toBase64() + "\"}" ); break;}
        }
    }else{
       blSize = -1;
    }

//    qint64 len =
    write(writeArr);
    if(verbouseMode)
        qDebug() << "write " << peerAddress() << peerPort() <<  writeArr.left(44);
//    qDebug() << "write SocketDlyaTrymacha " << QTime::currentTime().toString("hh:mm:ss.zzz");


    waitForBytesWritten(50);

}
//----------------------------------------------------------------------------------------------------------------------------
void SocketDlyaTrymacha::onDisconn()
{
    if(stopAll)
        return;

    stopAll = true;
     close();

    if(!mMac.isEmpty() && isMatildaDev == REM_DEV_MATILDA_DEV){
        isMatildaDev = REM_DEV_MATILDA_UNKNWN;
        emit removeMyId2Hash(mMac);
    }

    if(verbouseMode)
        qDebug() << "onDisconn SocketDlyaTrymacha " << myRemoteIpAndDescr << mIden << mMac;
    deleteLater();

}
//----------------------------------------------------------------------------------------------------------------------------
void SocketDlyaTrymacha::checkRemoteSha1()
{
    if(!macL4backupManager.isEmpty()){
        dtLastBackupCheck = QDateTime::currentDateTimeUtc().addSecs(12 * 60 * 60);
        QJsonObject j;
        j.insert("lHsh", lastSha1base64);
        mWrite2SocketJSON(j, COMMAND_CHECK_BACKUP_FILE_HASH_SUMM);

    }
}
//----------------------------------------------------------------------------------------------------------------------------
quint16 SocketDlyaTrymacha::startUploadBackup(const QString &serverIp, const QString &lastSha1base64)
{
    backupSessionId++;
    //запуск сервісу для вивантаження файлу резервної копії
    Service4uploadBackup *server = new Service4uploadBackup(verbouseMode, backupSessionId);
    SettLoader4svaha sLoader;
    quint16 startPort = sLoader.loadOneSett(SETT_SVAHA_DATA_START_PORT).toUInt();
    quint16 endPort = startPort + sLoader.loadOneSett(SETT_SVAHA_DATA_PORT_COUNT).toUInt();

    if(startPort > 65534 || endPort > 65534 || startPort < 1000 || endPort < 1000){
        startPort = SettLoader4svaha::defSETT_SVAHA_DATA_START_PORT();
        endPort = startPort + SettLoader4svaha::defSETT_SVAHA_DATA_PORT_COUNT();
    }


    quint16 svahaPort = server->findFreePort(startPort, endPort);
    if(verbouseMode)
        qDebug() << "start startUploadBackup" << svahaPort << backupSessionId;
    if(svahaPort == 0){
        server->deleteLater();

    }else{
        QThread *thread = new QThread(this);
        server->setWrite4aut(QCryptographicHash::hash(QString("%1\n\t%2\n\t%3\n\tAnnet\n\t").arg(QString::number(svahaPort)).arg(serverIp).arg(lastSha1base64).toLocal8Bit(), QCryptographicHash::Sha3_256).toBase64(QByteArray::OmitTrailingEquals), lastSha1base64);

        server->moveToThread(thread);
        connect(thread, SIGNAL(started()), server, SLOT(onThrdStarted()) );
        connect(server, SIGNAL(destroyed(QObject*)), thread, SLOT(quit()));
        connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()) );

        connect(this, SIGNAL(startTmrCheckRemoteSha1()), server, SLOT(onZombie()) );
        connect(server, SIGNAL(onSyncDone(quint8,QStringList,QString,QDateTime)), this, SLOT(onSyncDone(quint8,QStringList,QString,QDateTime)) );
        connect(server, SIGNAL(onSyncServiceDestr(quint8)), this, SLOT(onSyncServiceDestr(quint8)) );

        thread->start();
    }
    return svahaPort;
}
//----------------------------------------------------------------------------------------------------------------------------
void SocketDlyaTrymacha::mReadyReadF()
{
    int timeOutG = 45555, timeOut = 300;
    QByteArray readarr = readAll();
    QTime time;
    time.start();


    QTime timeG;
    timeG.start();

    int razivDuzkaL = 0;
    int razivDuzkaR = 0;
    bool wait4lapky = false;
    ReadJsonHelper::getRightLeftDuzka(razivDuzkaR, razivDuzkaL, wait4lapky, readarr);

    for( ; readarr.size() < MAX_PACKET_LEN && (readarr.isEmpty() || time.elapsed() < timeOut) && timeG.elapsed() < timeOutG; ){
        if(waitForReadyRead(10)){
            QByteArray arr = readAll();
            ReadJsonHelper::getRightLeftDuzka(razivDuzkaR, razivDuzkaL, wait4lapky, arr);
            readarr.append(arr);
            qDebug() << "razivDuzkaL razivDuzkaR " << razivDuzkaL << razivDuzkaR;
            time.restart();
        }

        if(razivDuzkaL > 0 && razivDuzkaR > 0){
            if(razivDuzkaR == razivDuzkaL)
                break;
        }
    }

    if(razivDuzkaL != razivDuzkaR || razivDuzkaL < 1){
        emit showMess("corrupted data.");
        return ;
    }else{

        int rIndx = 0, lIndx = 0;
        rIndx = ReadJsonHelper::indxOfRightDuzka(rIndx, readarr);
        rIndx++;
        for(int i = 0, iMax = 100; i < iMax; i++){

            decodeReadDataJSON(readarr.mid(lIndx, rIndx - lIndx ));
            lIndx = rIndx;
            rIndx = ReadJsonHelper::indxOfRightDuzka(rIndx, readarr);

            if(rIndx < 1)
                break;
            rIndx++;
        }


//        int duzkaIndx = readarr.indexOf("}");
//        int lastIndx = 0;

//        int len = readarr.length();
//        while(duzkaIndx > 1 && lastIndx < len){
//            decodeReadDataJSON(readarr.mid(lastIndx, duzkaIndx + 1));
//            duzkaIndx = readarr.indexOf("}", lastIndx);
//            lastIndx = duzkaIndx + 1;

//        }
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void SocketDlyaTrymacha::decodeReadDataJSON(const QByteArray &readArr)
{

    QJsonParseError jErr;
    QJsonDocument jDoc = QJsonDocument::fromJson( readArr, &jErr);

    QVariantHash hash = jDoc.object().toVariantHash();

    quint16 command = hash.take("cmd").toUInt();

    if(command == COMMAND_COMPRESSED_PACKET){

        if(messHshIsValid(jDoc.object(), readArr)){

            jDoc = QJsonDocument::fromJson( qUncompress( QByteArray::fromBase64( hash.value("zlib").toByteArray() )  ), &jErr);//qUncompress( <quint32 not compressed data len><comprssed data> )
            hash = jDoc.object().toVariantHash();
            command = hash.take("cmd").toUInt();

        }else{
            if(verbouseMode)
                qDebug() << tr("Received uncorrect request");
            emit showMess(tr("Received uncorrect request"));
            return;
        }

    }

    if(verbouseMode)
    qDebug() << "read:" << socketDescriptor() << peerAddress() << peerPort() << readArr ;


    if(!messHshIsValid(jDoc.object(), readArr)){
        if(verbouseMode)
            qDebug() << tr("Received uncorrect request");
        emit showMess(tr("Received uncorrect request"));
        return;
    }
    lastCommand = command;
    switch(command){

    case COMMAND_YOUR_ID_AND_MAC:{//активний сокет від пристрою опитування, один раз передає свої дані реєстарції
        if(isMatildaDev != REM_DEV_MATILDA_UNKNWN){
            mWrite2SocketJSON(errCodeLastOperationJson(command, ERR_INCORRECT_REQUEST), COMMAND_ERROR_CODE);
            isMatildaDev = REM_DEV_MATILDA_UNKNWN;
            QTimer::singleShot(5555, this, SLOT(onDisconn()) );
            break;
        }
        isMatildaDev = REM_DEV_MATILDA_DEV;
        mIden = hash.value("memo").toString();
        mMac.clear();
        QStringList macList = hash.value("mac").toString().toUpper().split("|", QString::SkipEmptyParts);

        for(int i = 0, iMax = macList.size(), j = 0; i < iMax && j < 10; i++){
            if(macList.at(i).startsWith("00:00:00:00:") || !macList.at(i).contains(":") || mMac.contains(macList.at(i)))
                continue;
            mMac.append(macList.at(i));
            j++;
        }

        if(mMac.isEmpty() || hash.value("mac").toString().remove("|").isEmpty()){
            if(verbouseMode)
                qDebug() << "bad mac " << socketDescriptor() << hash;
            onDisconn();
            return;
        }
        if(hash.value("cmprssn").toString().contains("zlib"))
            allowCompress = true;

        myRemoteIpAndDescr = this->peerAddress().toString();
        if(myRemoteIpAndDescr.left(7).toUpper() == "::FFFF:")
            myRemoteIpAndDescr = myRemoteIpAndDescr.mid(7);
        myRemoteIpAndDescr.append(QString(":%1").arg(socketDescriptor()));
        emit addMyId2Hash(mIden, mMac, myRemoteIpAndDescr, getObjIfo(hash.value("ao").toMap(), true), hash.contains("ao"));
        mWrite2SocketJSON(errCodeLastOperationJson(command, ERR_NO_ERROR), COMMAND_ERROR_CODE);//реєстрацію завершено упішно
        break;}

    case COMMAND_CONNECT_ME_2_THIS_ID_OR_MAC:{//запит від ПЗ конфігурації на з’єднання
        if(isMatildaDev == REM_DEV_MATILDA_DEV){
            mWrite2SocketJSON(errCodeLastOperationJson(command, ERR_INCORRECT_REQUEST), COMMAND_ERROR_CODE);
            return;
        }
        myRemoteIpAndDescr = this->peerAddress().toString();
        if(myRemoteIpAndDescr.left(7).toUpper() == "::FFFF:")
            myRemoteIpAndDescr = myRemoteIpAndDescr.mid(7);
        myRemoteIpAndDescr.append(QString(":%1").arg(socketDescriptor()));
        isMatildaDev = REM_DEV_MATILDA_CONF;
        bool isIDMode = hash.value("useId", false).toBool();
        QString str = hash.value("remote", "").toString();

        if(verbouseMode)
            qDebug() << "COMMAND_CONNECT_ME_2_THIS_ID_OR_MAC " << isIDMode << str;
        if(str.isEmpty()){
            mWrite2SocketJSON(errCodeLastOperationJson(command, ERR_INCORRECT_REQUEST), COMMAND_ERROR_CODE);
        }else{
            emit connMe2ThisIdOrMac(str, !isIDMode, myRemoteIpAndDescr, peerAddress().toString());
        }
        break;}

    case COMMAND_I_AM_A_ZOMBIE:{
        if(isMatildaDev == REM_DEV_MATILDA_DEV || isMatildaDev == REM_DEV_MATILDA_CONF){
            if(verbouseMode)
                qDebug() << "zombie killer" << peerAddress() << mIden << timeZombie.elapsed() / 1000;
            if(hash.contains("ao"))//періодично мені відправляються дані по пристрою
                emit infoAboutObj(mMac, getObjIfo(hash.value("ao").toMap(), false), mMac.size());



        }else{
            onDisconn();
        }

        if(timeZombie.elapsed() > 5000)
            mWrite2SocketJSON(QJsonObject(), COMMAND_I_AM_A_ZOMBIE);
        return;}


    case COMMAND_CHECK_BACKUP_FILE_HASH_SUMM:{
        if(verbouseMode)
            qDebug() << "COMMAND_CHECK_BACKUP_FILE_HASH_SUMM " << isMatildaDev << hash;
        if(isMatildaDev != REM_DEV_MATILDA_DEV){
            mWrite2SocketJSON(errCodeLastOperationJson(command, ERR_INCORRECT_REQUEST), COMMAND_ERROR_CODE);
            isMatildaDev = REM_DEV_MATILDA_UNKNWN;
            QTimer::singleShot(5555, this, SLOT(onDisconn()) );
            break;
        }


        if(hash.value("hshChngd", true).toBool()){
            emit infoAboutObj(mMac, getObjIfo(hash.value("ao").toMap(), false), mMac.size());


            QString serverIp = SettLoader4svaha().loadOneSett(SETT_MATILDA_DEV_IP).toString();
            if(serverIp.isEmpty())
                serverIp = "svaha.ddns.net";

            quint16 svahaPort = startUploadBackup(serverIp, hash.value("hsh").toString());
            if(verbouseMode)
                qDebug() << "hshChngd= " << hash.value("hsh").toString() << hash.value("ssn").toInt() << lastSha1base64 << svahaPort;

            if(svahaPort == 0){
                emit onSyncRequestRemoteSha1isEqual(macL4backupManager);
                dtLastBackupCheck = QDateTime::currentDateTimeUtc();
            }else{
                if(isMatildaDev == REM_DEV_MATILDA_DEV){
                    QJsonObject jObj;//створити з'єднання в режимі вивантаження резервної копії
                    jObj.insert("sIp", serverIp);
                    jObj.insert("sP", svahaPort);
                    jObj.insert("rIp", hash.value("hsh").toString());
                    jObj.insert("ssn", hash.value("ssn").toInt());
                    mWrite2SocketJSON(jObj, COMMAND_CONNECT_2_THIS_SERVICE);
                    return;
                }
                isMatildaDev = REM_DEV_MATILDA_UNKNWN;
            }


        }else{
            //конфігурація залишилась без змін,
            emit onSyncRequestRemoteSha1isEqual(macL4backupManager);
            dtLastBackupCheck = QDateTime::currentDateTimeUtc();
        }
        macL4backupManager.clear();

        break; }


    default: qDebug() << "unknown command " << command; break;

    }

    if(isMatildaDev == REM_DEV_MATILDA_DEV || isMatildaDev == REM_DEV_MATILDA_CONF)
        return;

    QTimer::singleShot(555, this, SLOT(onDisconn()));

}
//----------------------------------------------------------------------------------------------------------------------------
bool SocketDlyaTrymacha::isConnOpen()
{
    return (state() == QAbstractSocket::ConnectedState || state() == QAbstractSocket::ConnectingState);
}
//----------------------------------------------------------------------------------------------------------------------------
bool SocketDlyaTrymacha::messHshIsValid(const QJsonObject &jObj, QByteArray readArr)
{
    QStringList lh = getHshNames();
    int hshIndx = -1;

    QString hshName("");
    for(int i = 0, iMax = getHshNames().size(); i < iMax && hshIndx < 0; i++){
        if(jObj.contains(lh.at(i))){
            hshIndx = i;
            hshName = lh.at(i);
        }
    }

    if(hshIndx < 0){
        if(verbouseMode)
        qDebug() << "if(hshIndx < 0 " << hshIndx << readArr;
        return false;
    }else{
        lastHashSumm = hshIndx;
        int startIndx = readArr.indexOf(QString("\"%1\"").arg(hshName));
        QByteArray hshBase64;
        if(startIndx > 0){
            startIndx += hshName.length() + 4;
            int endIndx = readArr.indexOf("\"", startIndx + 1);
            hshBase64 = readArr.mid(startIndx , endIndx - startIndx);
            readArr = readArr.left(startIndx ) + "0" + readArr.mid(endIndx);
        }
        if(hshBase64.isEmpty())
            return false;

        hshBase64 = QByteArray::fromBase64(hshBase64);

        QByteArray myHash = QCryptographicHash::hash(readArr, static_cast<QCryptographicHash::Algorithm>(lastHashSumm));
        if(myHash == hshBase64){
            return true;
        }else{
            if(verbouseMode)
                qDebug() << "if(myHash != hshBase64 " << myHash.toBase64() << hshBase64.toBase64();
            return false;
        }
    }
}
//----------------------------------------------------------------------------------------------------------------------------
QStringHash SocketDlyaTrymacha::getObjIfo(const QVariantMap &h, const bool &addVersion)
{
    /*(if exists)
     *
     * SN <serial number>
     * vrsn
     * DEV
     * app
     *
     * //gsm section
     * IMEI
     * IMSI
     * CID
     * BAND
     * RSSI
     * ATI
     * RCSP
     * Ec/No
     *
     * //zigbee
     * ZCH
     * ZID
     * ZRSSI
     * LQI
     * VR
     * HV
     * Type
     * ZEUI64
     *
*/

    QStringHash hashAboutObj;
    if(!h.isEmpty()){
        QStringList lk = QString("SN vrsn DEV app IMEI IMSI CID LAC RSSI RCSP ATI Ec/No ZCH ZID ZRSSI LQI VR HV Type ZEUI64").split(" ");
        for(int i = 0, iMax = lk.size(); i < iMax; i++){
            if(h.contains(lk.at(i)) && !h.value(lk.at(i)).toString().isEmpty())
                hashAboutObj.insert(lk.at(i), h.value(lk.at(i)).toString());
        }
    }else{
        if(addVersion)
            hashAboutObj.insert("vrsn", QString::number(MATILDA_PROTOCOL_VERSION_V1));
    }
    lastAboutObj = hashAboutObj;

    if(verbouseMode)
        qDebug() << "hashAboutObj " << hashAboutObj << h;
    return hashAboutObj;
}
//----------------------------------------------------------------------------------------------------------------------------
QJsonArray SocketDlyaTrymacha::arrFromList(const QStringList &list)
{
    return QJsonArray::fromStringList(list);
}
//----------------------------------------------------------------------------------------------------------------------------
QString SocketDlyaTrymacha::hshSummName(const int &indx) const
{
    QStringList hashList = QString("Md4,Md5,Sha1,Sha224,Sha256,Sha384,Sha512,Sha3_224,Sha3_256,Sha3_384,Sha3_512").split(",");
    if(indx < 0 || indx >= hashList.size())
        return "Md5";

    return hashList.at(indx);
}
//----------------------------------------------------------------------------------------------------------------------------
QStringList SocketDlyaTrymacha::getHshNames() const
{
    return QString("Md4,Md5,Sha1,Sha224,Sha256,Sha384,Sha512,Sha3_224,Sha3_256,Sha3_384,Sha3_512").split(",");

}

//----------------------------------------------------------------------------------------------------------------------------
QJsonObject SocketDlyaTrymacha::errCodeLastOperationJson(const quint16 &command, const int &errCode) const
{
    QJsonObject j;
    j.insert("lcmd", command);
    j.insert("e", errCode);
    return j;
}
//----------------------------------------------------------------------------------------------------------------------------
QVariantHash SocketDlyaTrymacha::map2hash(const QVariantMap &map)
{
    QList<QString> listKeys = map.keys();
    QVariantHash h;
    for(int i = 0, iMax = listKeys.size(); i < iMax; i++)
        h.insert(listKeys.at(i), map.value(listKeys.at(i)));
    return h;
}
//----------------------------------------------------------------------------------------------------------------------------
