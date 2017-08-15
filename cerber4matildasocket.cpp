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
#include "cerber4matildasocket.h"
#include <QTimer>
#include <QHostAddress>
#include <QtCore>
#include <QStringList>
#include <QHostAddress>


#include "defcerberus.h"

//----------------------------------------------------------------------------------------------------------------------------
Cerber4matildaSocket::Cerber4matildaSocket(QObject *parent) : QTcpSocket(parent)
{
    stopAll = false;
    accessLevel = 0;
    hiSpeedConnection = false;
    connSpeed = 1;
}
//----------------------------------------------------------------------------------------------------------------------------
void Cerber4matildaSocket::writeTmpStamp(QVariantHash hash)
{
    myRemoteIpAndDescr = this->peerAddress().toString();
    if(myRemoteIpAndDescr.left(7).toUpper() == "::FFFF:")
        myRemoteIpAndDescr = myRemoteIpAndDescr.mid(7);
    myRemoteIpAndDescr.append(QString(":%1").arg(socketDescriptor()));

    hash.insert("version", MATILDA_PROTOCOL_VERSION_V2);


    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_6); //Qt_4_0);
    out << (quint32)0;
    out << hash;
    out.device()->seek(0);

    quint32 blSize = block.size();
    out << (quint32)(blSize - sizeof(quint32));

    tmStamp =  block;
    hash.insert("block", qCompress(tmStamp, 9));
    mWrite2Socket(hash, COMMAND_ZULU );
    timeZombie.start();
}
//----------------------------------------------------------------------------------------------------------------------------
void Cerber4matildaSocket::onThrdStarted()
{
    connect(this, SIGNAL(readyRead()), this, SLOT(mReadyRead()) );
    connect(this, SIGNAL(disconnected()), this, SLOT(onDisconn()) );

    SettLoader4svaha sLoader;

    QTimer *zombieTmr = new QTimer(this);
    zombieTmr->setSingleShot(true);
    zombieTmr->setInterval(sLoader.loadOneSett(SETT_ZOMBIE_MSEC).toInt());
    connect(zombieTmr, SIGNAL(timeout()), this, SLOT(onDisconn()) );
    connect(this, SIGNAL(disconnected()) , zombieTmr, SLOT(stop()) );
    connect(this, SIGNAL(readyRead()), zombieTmr, SLOT(start()) );
    zombieTmr->start();

    QTimer *killTmr = new QTimer(this);
    killTmr->setSingleShot(true);
    killTmr->setInterval( sLoader.loadOneSett(SETT_TIME_2_LIVE).toInt());// 24 * 60 * 60 * 1000);

    connect(killTmr, SIGNAL(timeout()) , this, SLOT(onDisconn()) );//максимальна тривалість 24 години
    connect(this, SIGNAL(disconnected()), killTmr, SLOT(stop()) );
}
//----------------------------------------------------------------------------------------------------------------------------
void Cerber4matildaSocket::remoteIdAndDevId(QStringHash hashMacRemoteId, QStringHash hashMacDevId, QStringHash hashTime, QString id, QStringHashHash hashAboutObject)
{

    if(id == myRemoteIpAndDescr){
        QVariantHash hash;

        QList<QString> listMac = hashMacRemoteId.keys();
        for(int i = 0, iMax = listMac.size(); i < iMax; i++){
            hash.insert( listMac.at(i), QStringList() << hashMacRemoteId.value(listMac.at(i)) << hashMacDevId.value(listMac.at(i)) << hashTime.value(listMac.at(i))
                         << SettLoader4svaha::strFromStrHash(hashAboutObject.value(listMac.at(i))) );
        }

        mWrite2Socket(hash, COMMAND_READ_HASH);
    }
}
//----------------------------------------------------------------------------------------------------------------------------
void Cerber4matildaSocket::mReadyRead()
{
    if(!(state() == QAbstractSocket::ConnectedState ))
        return;

    connSpeed = timeGalmo.elapsed() ;
    if(connSpeed > 0)
        connSpeed = lastByteWrt/connSpeed;

    if(connSpeed < NET_SPEED_VERY_LOW)
        connSpeed = NET_SPEED_VERY_LOW;

    qDebug() << "connection speed " << connSpeed << lastByteWrt << timeGalmo.elapsed();
    if(!hiSpeedConnection && timeGalmo.elapsed() < 150)
        hiSpeedConnection = true;


    while(timeGalmo.elapsed() < 35){
        this->waitForBytesWritten(7);
        this->waitForReadyRead(7);
    }


    quint32 blockSize;
    QDataStream inStrm(this);
    quint16 serverCommand;

    inStrm.setVersion(QDataStream::Qt_5_6);

    if(bytesAvailable() < (qint64)sizeof(quint32))
        return;

    inStrm >> blockSize;

    QTime timeG;
    timeG.start();

    int timeOutG = 90000;
    int timeOut = 1500;

    while(bytesAvailable() < blockSize && timeG.elapsed() < timeOutG){

        if(waitForReadyRead(timeOut))
            qDebug()<< "readServer1:"<< bytesAvailable() << blockSize;
    }

    if(bytesAvailable() < blockSize || blockSize > MAX_PACKET_LEN || bytesAvailable() > MAX_PACKET_LEN){

        qDebug()<< "readServer:" << blockSize << bytesAvailable() << readAll().toHex();
//        mWriteToSocket( errCodeLastOperation(0, ERR_CORRUPTED_DATA), COMMAND_ERROR_CODE);
        return;
    }


    if(bytesAvailable() == blockSize){

        inStrm >> serverCommand ;
        if(serverCommand == COMMAND_COMPRESSED_PACKET){
            QByteArray readArr;
            inStrm >> readArr;
            decodeReadData(uncompressRead(readArr, serverCommand), serverCommand);
        }else{
            QVariantHash readHash;
            inStrm >> readHash;
            decodeReadData(readHash, serverCommand);
        }

    }else{
        if(!inStrm.atEnd()){
            QVariant readVar;
            inStrm >> serverCommand >> readVar;
            if(serverCommand == COMMAND_COMPRESSED_PACKET){
                QByteArray readArr;
                inStrm >> readArr;
                decodeReadData(uncompressRead(readArr, serverCommand), serverCommand);
            }else{
                QVariantHash readHash;
                inStrm >> readHash;
                decodeReadData(readHash, serverCommand);
            }
            QTimer::singleShot(1, this, SLOT(mReadyRead()) );
            return;
        }
    }
}
//----------------------------------------------------------------------------------------------------------------------------
void Cerber4matildaSocket::mWrite2Socket(const QVariantHash s_data, const quint16 s_command)
{
    if(!isConnOpen())
        return;


    timeGalmo.restart();

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_6); //Qt_4_0);
    out << (quint32)0;
    out << s_command << s_data;

    out.device()->seek(0);

    quint32 blSize = block.size();


    if(blSize < SETT_MAX_UNCOMPRSS_PACkET_SIZE || connSpeed > NET_SPEED_NORMAL){
        out << (quint32)(blSize - sizeof(quint32));

    }else{
        block.clear();

        QByteArray blockUncompr;
        QDataStream outUncompr(&blockUncompr, QIODevice::WriteOnly);
        outUncompr.setVersion(QDataStream::Qt_5_6); //Qt_4_0);
        outUncompr << s_command << s_data;

        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_6); //Qt_4_0);
        out << (quint32)0;
        out << (quint16)COMMAND_COMPRESSED_PACKET << QByteArray(qCompress(blockUncompr,9));
        out.device()->seek(0);
//        qDebug() << "blSize " << blSize << s_command;
        blSize = block.size();
//        qDebug() << "blSize2 " << blSize;
        out << (quint32)(blSize - sizeof(quint32));
    }

    lastByteWrt = write(block);         //     qDebug() << block.toHex();
    waitForBytesWritten(50);
}
//----------------------------------------------------------------------------------------------------------------------------
void Cerber4matildaSocket::onDisconn()
{
    if(stopAll)
        return;

    emit removeCerverID(myRemoteIpAndDescr);
    stopAll = true;
    close();
    deleteLater();

}
//----------------------------------------------------------------------------------------------------------------------------
void Cerber4matildaSocket::decodeReadData(const QVariantHash &readHash, const quint16 &command)
{
    if(accessLevel != 1 && command != COMMAND_AUTHORIZE){
        onDisconn();
        return;
    }

    QVariantHash s_data;
    quint16 s_command = command;

    switch(command){

    case COMMAND_AUTHORIZE:{
        s_data.insert("a", authorizeF(readHash));

        qDebug() << "authorize " << accessLevel;

        if(accessLevel == 1){
           qDebug() << "auth good ";

        }else{
            onDisconn();
            return;
        }
        break;}

    case COMMAND_I_AM_A_ZOMBIE:{

        if(timeZombie.elapsed() < 15000)
            return;
        timeZombie.restart();
         break;}

    case COMMAND_READ_HASH:{
        emit getHashRemoteIdAndDevId(myRemoteIpAndDescr, readHash.value("auto", false).toBool()); return;}


    case COMMAND_WRITE_KILL_CLIENT:{
        emit killClientNow(readHash.value("id").toString(), readHash.value("byDevId").toBool());
        return;  }

    }
    mWrite2Socket(s_data, s_command);


}
//----------------------------------------------------------------------------------------------------------------------------
bool Cerber4matildaSocket::isConnOpen()
{
    return (state() == QAbstractSocket::ConnectedState || state() == QAbstractSocket::ConnectingState);
}

//----------------------------------------------------------------------------------------------------------------------------
quint8 Cerber4matildaSocket::authorizeF(QHash<QString, QVariant> h)
{
    accessLevel = 0;

    int version = h.value("version", 0xFFFF).toInt();
    QByteArray hashData = h.value("hsh", "").toByteArray();

    if(version > MATILDA_PROTOCOL_VERSION_V2 || tmStamp.isEmpty() || hashData.isEmpty()){
        accessLevel = 0;
        qDebug() << "MATILDA_PROTOCOL_VERSION_V2 not valid " << version << MATILDA_PROTOCOL_VERSION_V2;
        return accessLevel;
    }


    SettLoader4svaha sLoader;

    QVariantHash hashStrVar;// = sLoader.loadOneSett(SETT_SOME_SETT).toHash();
    QStringList listK = QString("root_l root_p operator_l operator_p guest_l  guest_p").split(" ", QString::SkipEmptyParts);
    QList<int> listKint;
    listKint << SETT_ADMIN_LOGIN << SETT_ADMIN_PASSWRD;

    for(int i = 0, iMax = listK.size(), iMax2 = listKint.size(); i < iMax && i < iMax2; i++){
        QByteArray arr = sLoader.loadOneSett(listKint.at(i)).toByteArray();

        hashStrVar.insert(listK.at(i), QCryptographicHash::hash(arr, QCryptographicHash::Sha3_256));

        qDebug() << i << arr << QCryptographicHash::hash(arr, QCryptographicHash::Sha3_256).toHex();
    }

//    if(hashStrVar.value("root_l").toByteArray().isEmpty())
//        hashStrVar.insert("root_l", QCryptographicHash::hash(QByteArray("admin"), QCryptographicHash::Sha3_256));

//    if(hashStrVar.value("operator_l").toByteArray().isEmpty())
//        hashStrVar.insert("operator_l", QCryptographicHash::hash(QByteArray("operator"), QCryptographicHash::Sha3_256));

//    if(hashStrVar.value("guest_l").toByteArray().isEmpty())
//        hashStrVar.insert("guest_l", QCryptographicHash::hash(QByteArray(""), QCryptographicHash::Sha3_256));


//    if(hashStrVar.value("root_p").toByteArray().isEmpty())
//        hashStrVar.insert("root_p", QCryptographicHash::hash(QByteArray(""), QCryptographicHash::Sha3_256));

//    if(hashStrVar.value("operator_p").toByteArray().isEmpty())
//        hashStrVar.insert("operator_p", QCryptographicHash::hash(QByteArray(""), QCryptographicHash::Sha3_256));

//    if(hashStrVar.value("guest_p").toByteArray().isEmpty())
//        hashStrVar.insert("guest_p", QCryptographicHash::hash(QByteArray(""), QCryptographicHash::Sha3_256));


    qDebug() << tmStamp.toHex();
    qDebug() << hashData.toHex();
    qDebug() << QCryptographicHash::hash(hashStrVar.value("root_l").toByteArray() + "\n" + tmStamp + "\n" + hashStrVar.value("root_p").toByteArray(), QCryptographicHash::Sha3_256).toHex();

    if(hashData == QCryptographicHash::hash(hashStrVar.value("root_l").toByteArray() + "\n" + tmStamp + "\n" + hashStrVar.value("root_p").toByteArray(), QCryptographicHash::Sha3_256)){
        accessLevel = 1;
    }else{
        if(hashData == QCryptographicHash::hash(hashStrVar.value("operator_l").toByteArray() + "\n" + tmStamp + "\n" + hashStrVar.value("operator_p").toByteArray(), QCryptographicHash::Sha3_256)){
            accessLevel = 2;
        }else{
            if(hashData == QCryptographicHash::hash(hashStrVar.value("guest_l").toByteArray() + "\n" + tmStamp + "\n" + hashStrVar.value("guest_p").toByteArray(), QCryptographicHash::Sha3_256)){
                accessLevel = 3;
            }
        }
    }

    tmStamp.clear();
    if(accessLevel > 0){
        qDebug() << "accessLevel , clear " << accessLevel ;

//        if(accessLevel < 3)
//            emit removeThisIpFromBlackList(SettLoader4matilda::showNormalIP(peerAddress().toString()));

    }else{
        qDebug() << "auth fail";
//        emit addThisIPToBlackList(SettLoader4matilda::showNormalIP(peerAddress().toString()));
            onDisconn();
    }
    return accessLevel;
}
//----------------------------------------------------------------------------------------------------------------------------
QVariantHash Cerber4matildaSocket::uncompressRead(QByteArray readArr, quint16 &command)
{
    qDebug() << "uncompress command=" << command;

    readArr = qUncompress(readArr);
    QDataStream outUncompr(readArr);
    outUncompr.setVersion(QDataStream::Qt_5_6); //Qt_4_0);
    QVariantHash readHash;
    outUncompr >> command >> readHash;
    qDebug() << "uncompress command2=" << command;
    return readHash;
}
//----------------------------------------------------------------------------------------------------------------------------
