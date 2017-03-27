/****************************************************************************
**
**   Copyright © 2016-2017 The KTS-INTEK Ltd.
**   Contact: http://www.kts-intek.com.ua
**
**  This file is part of svaha-service-kts.
**
**  svaha-service-kts is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  svaha-service-kts is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with svaha-service-kts.  If not, see <http://www.gnu.org/licenses/>.
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

#define REM_DEV_MATILDA_UNKNWN  -1
#define REM_DEV_MATILDA_DEV     0
#define REM_DEV_MATILDA_CONF    1

//----------------------------------------------------------------------------------------------------------------------------
SocketDlyaTrymacha::SocketDlyaTrymacha(QObject *parent) : QTcpSocket(parent)
{    



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
        qDebug() << "startConn 111" << peerAddress() << peerPort() << objId << objMac << objSocketId;
        qDebug() << "startConn 222" <<  mIden << mMac << myRemoteIpAndDescr;
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
        emit addMyId2Hash(mIden,QStringList() << mac, myRemoteIpAndDescr);
        qDebug() << "checkThisMac " << mac << mIden << myRemoteIpAndDescr;
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
void SocketDlyaTrymacha::mReadyRead()
{
    int timeOutG = 45555, timeOut = 300;
    QByteArray readarr = readAll();
    QTime time;
    time.start();
    int razivDuzkaL = readarr.count('{'), razivDuzkaR = readarr.count('}');

    QTime timeG;
    timeG.start();

    while( razivDuzkaR < razivDuzkaL && readarr.size() < MAX_PACKET_LEN && time.elapsed() < timeOut && timeG.elapsed() < timeOutG && razivDuzkaL > 0){
        if(waitForReadyRead(timeOut)){
            time.restart();
            readarr.append(readAll());
            razivDuzkaL = readarr.count('{');
            razivDuzkaR = readarr.count('}');
//            qDebug() << "razivDuzkaL razivDuzkaR " << razivDuzkaL << razivDuzkaR;
        }
    }

    if(razivDuzkaL != razivDuzkaR || razivDuzkaL < 1){
        emit showMess("corrupted data.");
        qDebug()<< "readServer:"<< readarr;
        return ;
    }else{
        int duzkaIndx = readarr.indexOf("}");
        int lastIndx = 0;

        int len = readarr.length();
        while(duzkaIndx > 1 && lastIndx < len){
            decodeReadDataJSON(readarr.mid(lastIndx, duzkaIndx + 1));
            duzkaIndx = readarr.indexOf("}", lastIndx);
            lastIndx = duzkaIndx + 1;

        }
    }

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

//    qDebug() << "write " << peerAddress() << peerPort() << len << writeArr;
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

    qDebug() << "onDisconn SocketDlyaTrymacha " << myRemoteIpAndDescr << mIden << mMac;
    deleteLater();

}

//----------------------------------------------------------------------------------------------------------------------------
void SocketDlyaTrymacha::decodeReadDataJSON(const QByteArray &readArr)
{

    QJsonParseError jErr;
    QJsonDocument jDoc = QJsonDocument::fromJson( readArr, &jErr);

    QVariantHash hash = map2hash(jDoc.object().toVariantMap());

    quint16 command = hash.take("cmd").toUInt();

    if(command == COMMAND_COMPRESSED_PACKET){

        if(messHshIsValid(jDoc.object(), readArr)){

            jDoc = QJsonDocument::fromJson( qUncompress( QByteArray::fromBase64( hash.value("zlib").toByteArray() )  ), &jErr);//qUncompress( <quint32 not compressed data len><comprssed data> )
            hash = map2hash(jDoc.object().toVariantMap());

            command = hash.take("cmd").toUInt();

        }else{
            qDebug() << tr("Received uncorrect request");
            emit showMess(tr("Received uncorrect request"));
            return;
        }

    }

//    qDebug() << "read:" << socketDescriptor() << peerAddress() << peerPort() << readArr;


    if(!messHshIsValid(jDoc.object(), readArr)){
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
            break;
        }
        isMatildaDev = REM_DEV_MATILDA_DEV;
        mIden = hash.value("memo").toString();
        mMac = hash.value("mac").toString().toUpper().split("|", QString::SkipEmptyParts);
        if(mMac.isEmpty() || hash.value("mac").toString().remove("|").isEmpty()){
            onDisconn();
            return;
        }
        if(hash.value("cmprssn").toString().contains("zlib"))
            allowCompress = true;
        myRemoteIpAndDescr = this->peerAddress().toString();
        if(myRemoteIpAndDescr.left(7).toUpper() == "::FFFF:")
            myRemoteIpAndDescr = myRemoteIpAndDescr.mid(7);
        myRemoteIpAndDescr.append(QString(":%1").arg(socketDescriptor()));
        emit addMyId2Hash(mIden,mMac,myRemoteIpAndDescr);
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

        qDebug() << "COMMAND_CONNECT_ME_2_THIS_ID_OR_MAC " << isIDMode << str;
        if(str.isEmpty()){
            mWrite2SocketJSON(errCodeLastOperationJson(command, ERR_INCORRECT_REQUEST), COMMAND_ERROR_CODE);
        }else{
            emit connMe2ThisIdOrMac(str, !isIDMode, myRemoteIpAndDescr, peerAddress().toString());
        }
        break;}

    case COMMAND_I_AM_A_ZOMBIE:{
        if(isMatildaDev == REM_DEV_MATILDA_DEV || isMatildaDev == REM_DEV_MATILDA_CONF){
            qDebug() << "zombie killer" << peerAddress() << mIden << timeZombie.elapsed() / 1000;
        }else{
            onDisconn();
        }

        if(timeZombie.elapsed() > 5000)
            mWrite2SocketJSON(QJsonObject(), COMMAND_I_AM_A_ZOMBIE);
        return;}

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
//            qDebug() << "hash is " << hshName;
        }
    }

    if(hshIndx < 0){
        qDebug() << "if(hshIndx < 0 " << hshIndx;
        return false;
    }else{
        lastHashSumm = hshIndx;
        int startIndx = readArr.indexOf(QString("\"%1\"").arg(hshName));
        QByteArray hshBase64;
        if(startIndx > 0){
            startIndx += hshName.length() + 4;
//            qDebug() << "hshName " << hshName << startIndx << readArr.mid(startIndx);

            int endIndx = readArr.indexOf("\"", startIndx + 1);
//            qDebug() << "endIndx " << endIndx << readArr.mid(endIndx);

            hshBase64 = readArr.mid(startIndx , endIndx - startIndx);
//            qDebug() << hshBase64;
            readArr = readArr.left(startIndx ) + "0" + readArr.mid(endIndx);
//            qDebug() << readArr;

//            qDebug() << QCryptographicHash::hash("helloworld", QCryptographicHash::Md5).toHex();
        }
        if(hshBase64.isEmpty())
            return false;

        hshBase64 = QByteArray::fromBase64(hshBase64);

        QByteArray myHash = QCryptographicHash::hash(readArr, static_cast<QCryptographicHash::Algorithm>(lastHashSumm));
        if(myHash == hshBase64){
            return true;
        }else{
            qDebug() << "if(myHash != hshBase64 " << myHash.toBase64() << hshBase64.toBase64();
            return false;
        }
    }
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
