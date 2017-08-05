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

#include "socket4uploadbackup.h"
#include "../matilda-bbb/moji_defy.h"
#include <QJsonObject>
#include <QtCore>

#include <QTimer>
//---------------------------------------------------------------------------------
Socket4uploadBackup::Socket4uploadBackup(QByteArray write4authorize, QObject *parent) : QTcpSocket(parent)
{
    matildaLogined = false;
    dataStreamVersion = QDataStream::Qt_DefaultCompiledVersion;
    stopAfter = false;
    stopAll = false;
    zombieRetr = 0;

    this->write4authorize = write4authorize;
    connect(this, SIGNAL(readyRead()), this, SLOT(mReadyRead()) );
    connect(this, SIGNAL(disconnected()), this, SLOT(onDisconn()) );
    if(bytesAvailable() > 0)
        QTimer::singleShot(11, this, SLOT(mReadyRead()) );

}


//---------------------------------------------------------------------------------
void Socket4uploadBackup::onDisconn()
{
    emit iAmDisconn();
    close();
    deleteLater();
}
//---------------------------------------------------------------------------------
void Socket4uploadBackup::mReadyRead()
{
    disconnect(this, SIGNAL(readyRead()), this, SLOT(mReadyRead()) );
    mReadyReadF();
    connect(this, SIGNAL(readyRead()), SLOT(mReadyRead()) );
}
//---------------------------------------------------------------------------------
void Socket4uploadBackup::mReadyReadF()
{
    int timeOut = 30000;
    int timeOutG = 90000;

    if(!matildaLogined){
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
        qDebug()<< "readServer2:"<< readAll();
        return;
    }


    quint32 blockSize;
    QDataStream inStrm(this);

    inStrm.setVersion(dataStreamVersion);
    if(this->bytesAvailable() < (int)sizeof(quint32))
        return ;

    inStrm >> blockSize;

    QTime timeG;
    timeG.start();
    while(bytesAvailable() < blockSize && timeG.elapsed() < timeOutG)
        waitForReadyRead(timeOut);


    if(bytesAvailable() < blockSize){
        qDebug()<< "readServer:"<< bytesAvailable() << blockSize << readAll().toHex();
        return ;
    }

    stopAfter = true;

    if(bytesAvailable() == blockSize){

        qDebug() << "good byte " << bytesAvailable() << blockSize ;
        quint16 serverCommand;
        QVariant readVar;

        inStrm >> serverCommand >> readVar;
        if(serverCommand == COMMAND_COMPRESSED_PACKET)
            decodeReadData(uncompressRead(readVar.toByteArray(), serverCommand, blockSize) , serverCommand);
        else{
            decodeReadData(readVar, serverCommand);
             emit changeCounters( blockSize, -1, true);
        }
        lastReadData = readVar;
        lastServerCommand = serverCommand;
        stopAfter = false;
    }else{
        if(!inStrm.atEnd()){
            quint16 serverCommand;
            QVariant readVar;
            qDebug() << "not good byte " << bytesAvailable() << blockSize ;

            inStrm >> serverCommand >> readVar;
            if(serverCommand == COMMAND_COMPRESSED_PACKET)
                decodeReadData(uncompressRead(readVar.toByteArray(), serverCommand, bytesAvailable()) , serverCommand);
            else{
                decodeReadData(readVar, serverCommand);
                emit changeCounters( blockSize, -1 , true);

            }
            lastReadData = readVar;
            lastServerCommand = serverCommand;
            stopAfter = false;
            QTimer::singleShot(1, this, SLOT(mReadyRead()) );

//            inStrm >> blockSize;

//            if(blockSize > MAX_PACKET_LEN || !readVar.isValid())
        }
    }
    stopAll = stopAfter;
}
//---------------------------------------------------------------------------------
void Socket4uploadBackup::decodeReadData(const QVariant &dataVar, const quint16 &command)
{
    if(COMMAND_READ_DATABASE_GET_VAL != command && COMMAND_READ_DATABASE != command && command != COMMAND_READ_METER_LIST_FRAMED && command != COMMAND_WRITE_DROP_TABLE){
        qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") <<  " read:" << command;
        qDebug() << "decodeReadData" << dataVar.isValid() ;//<< dataVar;
    }

    if(!dataVar.isValid()){
        qDebug() << "!dataVar.isValid";
        return;
    }

    zombieRetr = 0;
    if(command == COMMAND_I_AM_A_ZOMBIE){
        qDebug() << "zombie echo" << peerAddress();
//        mWriteToSocket("", COMMAND_I_AM_A_ZOMBIE);
        return;
    }

    switch(command){

    case COMMAND_AUTHORIZE:{
        qDebug() << "access = " << dataVar.toHash();

        QVariantHash h = dataVar.toHash();

        accessLevel = h.value("a").toUInt();

        if(accessLevel == MTD_USER_BACKUP){
            hashAboutObj = h.value("ao").toHash();


        }else{
            onDisconn();
        }

        return;}

    }

}
//---------------------------------------------------------------------------------
void Socket4uploadBackup::decodeReadDataJSON(const QByteArray &dataArr)
{
    QJsonParseError jErr;
    QJsonDocument jDoc = QJsonDocument::fromJson( dataArr, &jErr);

    QVariantHash hash = jDoc.object().toVariantHash();
    qDebug() << hash;

    quint16 command = hash.take("cmd").toUInt();

    if(command == COMMAND_COMPRESSED_PACKET){
        if(messHshIsValid(jDoc.object(), dataArr)){
            jDoc = QJsonDocument::fromJson( qUncompress( QByteArray::fromBase64( hash.value("zlib").toByteArray() )  ), &jErr);//qUncompress( <quint32 not compressed data len><comprssed data> )
            hash = hash = jDoc.object().toVariantHash();

            command = hash.take("cmd").toUInt();
        }else{
            onDisconn();
            return;
        }
    }

    qDebug() << "decodeReadData" << command;
    qDebug()  << jDoc.object();

    if(!messHshIsValid(jDoc.object(), dataArr)){
        onDisconn();
        return;
    }

    zombieRetr = 0;
    switch(command){
    case COMMAND_ZULU:{


        if(hash.value("name").toString() == "Matilda" && hash.value("version").toInt() > 0 && QDateTime::fromString(hash.value("UTC").toString(), "yyyy-MM-dd hh:mm:ss").isValid()){
            if(!hash.value("err").toString().isEmpty()){
                onDisconn();
                return;
            }else{

                if(hash.value("QDS").toInt() >= QDataStream::Qt_4_8 && hash.value("QDS").toInt() <= QDataStream::Qt_DefaultCompiledVersion){


                    matildaLogined = true;
                    dataStreamVersion = hash.value("QDS").toInt();

                    QJsonObject jObj;
                    jObj.insert("version", hash.value("version").toInt());
                    jObj.insert("hsh", write4authorize);// QString(QCryptographicHash::hash(loginPasswd.at(0) + "\n" + dataArr + "\n" + loginPasswd.at(1), QCryptographicHash::Sha3_256).toBase64()));
//
                    //mode JSON and QDataStream
                    jObj.insert("QDS", QString::number(dataStreamVersion));//активація режиму QDataStream
                    jObj.insert("cmmprssn", "zlib");
                    jObj.insert("pos", "up");//only 4 backup
                    stopAll = false;
                    mWrite2SocketJSON(jObj, COMMAND_AUTHORIZE, 2);
                    return;
                }

            }
        }
        onDisconn();
        break;}

    case COMMAND_I_AM_A_ZOMBIE:{
        qDebug() << "zombie echo" << peerAddress();
//        mWriteToSocket("", COMMAND_I_AM_A_ZOMBIE);
        return;}


    default: qDebug() << "Socket4uploadBackup json !command=" << command; onDisconn(); break;
    }
}
//---------------------------------------------------------------------------------
bool Socket4uploadBackup::messHshIsValid(const QJsonObject &jObj, QByteArray readArr)
{
    QStringList lh = getHshNames();
    int hshIndx = -1;
    int lastHashSumm = 1;
    QString hshName("");
    for(int i = 0, iMax = getHshNames().size(); i < iMax && hshIndx < 0; i++){
        if(jObj.contains(lh.at(i))){
            hshIndx = i;
            hshName = lh.at(i);
            qDebug() << "hash is " << hshName;
        }
    }


    if(hshIndx < 0){
        qDebug() << "if(hshIndx < 0 " << hshIndx;
        return false;
    }else{
        lastHashSumm = hshIndx;
        int startIndx = readArr.indexOf(QString("\"%1\":").arg(hshName));
        QByteArray hshBase64;
        if(startIndx > 0){
            startIndx += hshName.length() + 4;
            qDebug() << "hshName " << hshName << startIndx << readArr.mid(startIndx);

            int endIndx = readArr.indexOf("\"", startIndx + 1);
            qDebug() << "endIndx " << endIndx << readArr.mid(endIndx);

            hshBase64 = readArr.mid(startIndx , endIndx - startIndx);
            qDebug() << hshBase64;
            readArr = readArr.left(startIndx ) + "0" + readArr.mid(endIndx);
            qDebug() << readArr;

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

//---------------------------------------------------------------------------------
QStringList Socket4uploadBackup::getHshNames() const
{
    return QString("Md4,Md5,Sha1,Sha224,Sha256,Sha384,Sha512,Sha3_224,Sha3_256,Sha3_384,Sha3_512").split(",");
}
//---------------------------------------------------------------------------------
qint64 Socket4uploadBackup::mWrite2SocketJSON(QJsonObject jObj, const quint16 s_command, int lastHashSumm)
{
    jObj.insert("cmd", s_command);
    stopAll = false;
    bool allowCompress = true;


    QByteArray writeArr ;
    if(true){
        QJsonDocument jDoc2DST(jObj);
        writeArr = jDoc2DST.toJson(QJsonDocument::Compact);
        writeArr.chop(1);// remove }
    }

    qDebug() << "writeArr0 " << writeArr;
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

        qDebug() << "writeArr1 comprs " << writeArr;
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

    readAll();
    qint64 len = write(writeArr);
    qDebug() << "writeJSON " << QTime::currentTime().toString("hh:mm:ss.zzz");
    qDebug() << writeArr;

    waitForBytesWritten(50);

    return len;
}
//---------------------------------------------------------------------------------
QVariant Socket4uploadBackup::uncompressRead(QByteArray readArr, quint16 &command, qint64 lenBefore)
{
    qDebug() << "uncompress command=" << command << readArr.size();

    readArr = qUncompress(readArr);
     emit changeCounters( lenBefore, readArr.length() , true);
    QVariant readVar;
    QDataStream outUncompr(readArr);
    outUncompr.setVersion(dataStreamVersion); //Qt_4_0);


    outUncompr >> command >> readVar;
    qDebug() << "uncompress command2=" << command << readArr.size();
    return readVar;
}
//---------------------------------------------------------------------------------
QByteArray Socket4uploadBackup::varHash2arr(const QVariantHash &hash)
{
    //було для ПЗ
//    QByteArray block;
//    QDataStream out(&block, QIODevice::WriteOnly);
//    out.setVersion(QDataStream::Qt_5_6); //Qt_4_0);
//    out << (quint32)0;
//    out << hash;
//    out.device()->seek(0);
//    out << (quint32)(block.size() - sizeof(quint32));

//    QByteArray arr = qCompress(block,9);
//    qDebug() << "block.size " << block.size() << arr.size() << arr.left(4).toHex() << arr.left(4).toHex().toUInt(0, 16);

    //варіант для резервних копій, те саме тепер для ПЗ
    QByteArray writeArr;
    QDataStream out(&writeArr,QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_6);
    out << (quint64)0 << hash;
    out.device()->seek(0);
    quint64 blSize = writeArr.size();
    out << (quint64)(blSize );
    writeArr = qCompress(writeArr,9);


    return writeArr;
}
//---------------------------------------------------------------------------------
void Socket4uploadBackup::mWriteToSocket(const QVariant s_data, const quint16 s_command)
{
    if(!isOpen()){
        qDebug() << "matildaclient::mWriteToSocket connIsDown " << QTime::currentTime().toString("hh:mm:ss.zzz") << isOpen() << state();
        QTimer::singleShot(11, this, SLOT(onDisconn()));
        return;
    }

    stopAll = false;

    if(s_command == COMMAND_ERROR_CODE || s_command == COMMAND_ERROR_CODE_EXT){
        qDebug() << "block write " << s_command << s_data;
        return;
    }
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(dataStreamVersion); //Qt_4_0);
    out << (quint32)0;
    out << s_command << s_data;
    out.device()->seek(0);

//     out << (quint32)(block.size() - sizeof(quint32));
    quint32 blSize = block.size();



    if(blSize < SETT_MAX_UNCOMPRSS_PACkET_SIZE || s_command == COMMAND_WRITE_UPGRADE || s_command == COMMAND_COMPRESSED_STREAM){
        out << (quint32)(blSize - sizeof(quint32));

    }else{
        block.clear();

        QByteArray blockUncompr;
        QDataStream outUncompr(&blockUncompr, QIODevice::WriteOnly);
        outUncompr.setVersion(dataStreamVersion); //Qt_4_0);
        outUncompr << s_command << s_data;

        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(dataStreamVersion); //Qt_4_0);
        out << (quint32)0;
        out << (quint16)COMMAND_COMPRESSED_PACKET << QVariant(qCompress(blockUncompr,9));
        out.device()->seek(0);
        qDebug() << "blSize " << blSize;
        quint32 blSize2 = block.size();
        qDebug() << "blSize2 " << blSize2;
        out << (quint32)(blSize2 - sizeof(quint32));
    }

    qint64 len = write(block);
    qDebug() << "write " << QTime::currentTime().toString("hh:mm:ss.zzz") << len << s_command;

    waitForBytesWritten(50);

}
//---------------------------------------------------------------------------------
