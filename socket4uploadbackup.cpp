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
#include "settloader4svaha.h"

#include <QTimer>
#include <QHostAddress>

//---------------------------------------------------------------------------------
Socket4uploadBackup::Socket4uploadBackup(const bool &verboseMode, QString write4authorizeBase64, QString lastSha1base64, QObject *parent) : QTcpSocket(parent)
{
    matildaLogined = false;
    serverAuthDone = false;
    dataStreamVersion = QDataStream::Qt_DefaultCompiledVersion;
    stopAfter = false;
    stopAll = false;
    zombieRetr = 0;
    this->lastSha1base64 = lastSha1base64;
    this->verboseMode = verboseMode;
    this->write4authorizeBase64 = write4authorizeBase64;
    connect(this, SIGNAL(readyRead()), this, SLOT(mReadyRead()) );
    connect(this, SIGNAL(disconnected()), this, SLOT(onDisconn()) );
    if(bytesAvailable() > 0)
        QTimer::singleShot(11, this, SLOT(mReadyRead()) );
    dtCreatedBackupUtc = QDateTime::currentDateTimeUtc();
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
            if(verboseMode) qDebug()<< "readServer:"<< readarr;
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
        if(verboseMode) qDebug()<< "readServer2:"<< readAll();
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
        if(verboseMode) qDebug()<< "readServer:"<< bytesAvailable() << blockSize << readAll().toHex();
        return ;
    }

    stopAfter = true;

    if(bytesAvailable() == blockSize){

        if(verboseMode) qDebug() << "good byte " << bytesAvailable() << blockSize ;
        quint16 serverCommand;
        QVariant readVar;

        inStrm >> serverCommand >> readVar;
        if(serverCommand == COMMAND_COMPRESSED_PACKET)
            decodeReadData(uncompressRead(readVar.toByteArray(), serverCommand, blockSize) , serverCommand);
        else{
            decodeReadData(readVar, serverCommand);
        }
        stopAfter = false;
    }else{
        if(!inStrm.atEnd()){
            quint16 serverCommand;
            QVariant readVar;
            if(verboseMode) qDebug() << "not good byte " << bytesAvailable() << blockSize ;

            inStrm >> serverCommand >> readVar;
            if(serverCommand == COMMAND_COMPRESSED_PACKET)
                decodeReadData(uncompressRead(readVar.toByteArray(), serverCommand, bytesAvailable()) , serverCommand);
            else{
                decodeReadData(readVar, serverCommand);

            }
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
    if(verboseMode){
        qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") <<  " read:" << command;
        qDebug() << "Socket4uploadBackup decodeReadData" << dataVar.isValid() ;//<< dataVar;
    }

    if(!dataVar.isValid()){
        qDebug() << "Socket4uploadBackup !dataVar.isValid";
        return;
    }

    zombieRetr = 0;
    if(command == COMMAND_I_AM_A_ZOMBIE){
        if(verboseMode) qDebug() << "zombie echo" << peerAddress();
//        mWriteToSocket("", COMMAND_I_AM_A_ZOMBIE);
        return;
    }

    switch(command){

    case COMMAND_AUTHORIZE:{
        if(verboseMode)  qDebug() << "Socket4uploadBackup access = " << dataVar.toHash();

        QVariantHash h = dataVar.toHash();

        accessLevel = h.value("a").toUInt();
        QByteArray serviceAccessKey = QCryptographicHash::hash("$Try2Annet$\t\n\r "+ write4authorizeBase64.toLocal8Bit() + " \r\n\t$Try2Annet$", QCryptographicHash::Sha3_256);

        if(accessLevel == MTD_USER_BACKUP && serviceAccessKey == h.value("sak").toByteArray()){
            hashAboutObj = h.value("ao").toHash();
            backupArr.clear();
            backupArrLen = 0;
            serverAuthDone = true;
           mWriteToSocket(QVariantHash(), COMMAND_GET_CACHED_BACKUP);//start read
        }else{
            onDisconn();
        }

        return;}

    case COMMAND_GET_CACHED_BACKUP:{
        if(!serverAuthDone){
            onDisconn();
            return;
        }
        QVariantHash h = dataVar.toHash();
        if(h.contains("t"))
            backupArrLen = (qint32)h.value("t").toInt();
        qint32 pos = h.value("i").toInt();

        backupArr.append(h.value("d").toByteArray());
        if(pos < 0 || !h.contains("d")){
            //закінчено зчитування
            if(backupArr.length() == backupArrLen && backupArrLen > 0){
                saveBackupArrAsFile();
            }
            break;
        }else{
            h.clear();
            h.insert("i", pos);
            mWriteToSocket(h, COMMAND_GET_CACHED_BACKUP);
        }

        return;}

    }
    QTimer::singleShot(11, this, SLOT(onDisconn()));
}
//---------------------------------------------------------------------------------
void Socket4uploadBackup::decodeReadDataJSON(const QByteArray &dataArr)
{
    QJsonParseError jErr;
    QJsonDocument jDoc = QJsonDocument::fromJson( dataArr, &jErr);

    QVariantHash hash = jDoc.object().toVariantHash();
    if(verboseMode) qDebug() << hash;

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

    if(verboseMode){
        qDebug() << "Socket4uploadBackup decodeReadData" << command;
        qDebug()  << jDoc.object();
    }
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
                    jObj.insert("hsh", write4authorizeBase64);// QString(QCryptographicHash::hash(loginPasswd.at(0) + "\n" + dataArr + "\n" + loginPasswd.at(1), QCryptographicHash::Sha3_256).toBase64()));
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
        if(verboseMode) qDebug() << "zombie echo" << peerAddress();
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
            if(verboseMode) qDebug() << "hash is " << hshName;
        }
    }


    if(hshIndx < 0){
        if(verboseMode) qDebug() << "if(hshIndx < 0 " << hshIndx;
        return false;
    }else{
        lastHashSumm = hshIndx;
        int startIndx = readArr.indexOf(QString("\"%1\":").arg(hshName));
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
            if(verboseMode) qDebug() << "if(myHash != hshBase64 " << myHash.toBase64() << hshBase64.toBase64();
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

    if(verboseMode) qDebug() << "writeArr0 " << writeArr;
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

        if(verboseMode) qDebug() << "writeArr1 comprs " << writeArr;
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
    if(verboseMode){
        qDebug() << "writeJSON " << QTime::currentTime().toString("hh:mm:ss.zzz");
        qDebug() << writeArr;
    }
    waitForBytesWritten(50);

    return len;
}
//---------------------------------------------------------------------------------
QVariant Socket4uploadBackup::uncompressRead(QByteArray readArr, quint16 &command, qint64 lenBefore)
{
    if(verboseMode) qDebug() << "uncompress command=" << command << readArr.size() << lenBefore;

    readArr = qUncompress(readArr);
    QVariant readVar;
    QDataStream outUncompr(readArr);
    outUncompr.setVersion(dataStreamVersion); //Qt_4_0);


    outUncompr >> command >> readVar;
    if(verboseMode) qDebug() << "uncompress command2=" << command << readArr.size();
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
        if(verboseMode) qDebug() << "matildaclient::mWriteToSocket connIsDown " << QTime::currentTime().toString("hh:mm:ss.zzz") << isOpen() << state();
        QTimer::singleShot(11, this, SLOT(onDisconn()));
        return;
    }

    stopAll = false;

    if(s_command == COMMAND_ERROR_CODE || s_command == COMMAND_ERROR_CODE_EXT){
        if(verboseMode) qDebug() << "block write " << s_command << s_data;
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
        quint32 blSize2 = block.size();
        out << (quint32)(blSize2 - sizeof(quint32));
    }

    qint64 len = write(block);
    if(verboseMode) qDebug() << "write " << QTime::currentTime().toString("hh:mm:ss.zzz") << len << s_command;
    waitForBytesWritten(50);

}
//---------------------------------------------------------------------------------
void Socket4uploadBackup::saveBackupArrAsFile()
{
    if(lastSha1base64.isEmpty())
        lastSha1base64 = QCryptographicHash::hash(backupArr, QCryptographicHash::Sha1).toBase64(QByteArray::OmitTrailingEquals);// toHex().toLower();// toBase64(QByteArray::OmitTrailingEquals);
    if(lastSha1base64.isEmpty() || backupArr.isEmpty()){
        if(verboseMode)
            qDebug() << "noSha1=" << lastSha1base64.isEmpty() << ", noData=" << backupArr.isEmpty() << ", dataLen=" << backupArrLen;
        return;
    }

    //create new backup
    //<work dir>/<year>/<month>/<file names>  //UTC date time!!!
    //file name <base64 ( omit "=",  replace("/", "=", on file only)) sha1>_<mac(X)>_...other keys

    QString workDir = SettLoader4svaha().loadOneSett(SETT_SYNC_WORKDIR).toString();
    if(workDir.isEmpty()){
        if(verboseMode)
            qDebug() << "workDir is not valid " << workDir;
        return;
    }

    workDir = QString("%1/%2/%3").arg(workDir).arg(dtCreatedBackupUtc.date().year()).arg(dtCreatedBackupUtc.date().month());
    QDir dir(workDir);
    if(!dir.exists())
        dir.mkpath(workDir);

    QStringList macL;
    QString fileName = lastSha1base64 + "_" + fileNameFromAboutObject(macL, lastSha1base64.length());
    fileName = fileName.replace("/", "=");

    QSaveFile sFile(workDir + "/" + fileName);
    if(sFile.open(QSaveFile::WriteOnly|QSaveFile::Unbuffered)){
        sFile.write(backupArr);
        if(sFile.commit()){
            if(verboseMode)
                qDebug() << "backup saved " << workDir << fileName ;

            emit onSyncDone(macL, lastSha1base64, dtCreatedBackupUtc);
        }else{
            if(verboseMode)
                qDebug() << "can't save file " << workDir << fileName << sFile.errorString();
        }
    }else{
        qDebug() << "Socket4uploadBackup can't creat file " << fileName << sFile.errorString();
    }

}
//---------------------------------------------------------------------------------
QString Socket4uploadBackup::fileNameFromAboutObject(QStringList &macL, const int &shaLen)
{
    /*(if exists)
     * ID
     * SN <serial number>
     * vrsn
     * DEV
     * app
     * MAC<x>   0 < x < 10
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
     * EUI64
     *
*/

    //file name <base64 ( omit "=",  replace("/", "=")) sha1>_<mac(X)>_...other keys

    QVariantHash hashAboutObj = this->hashAboutObj;
    //make some optimisation
    if(hashAboutObj.contains("ID") && hashAboutObj.value("ID").toString().length() > 50)
        hashAboutObj.insert("ID", hashAboutObj.value("ID").toString().left(50));

    if(hashAboutObj.contains("RSSI"))
        hashAboutObj.insert("RSSI", hashAboutObj.value("RSSI").toString().split("_").first());

    if(hashAboutObj.contains("RCSP"))
        hashAboutObj.insert("RCSP", hashAboutObj.value("RCSP").toString().split("_").first());

    if(hashAboutObj.contains("ZID"))
        hashAboutObj.insert("ZID", hashAboutObj.value("ZID").toString().mid(2));

    if(hashAboutObj.contains("ZCH"))
        hashAboutObj.insert("ZCH", hashAboutObj.value("ZCH").toString().split("_").last().mid(2));

    if(hashAboutObj.contains("ZRSSI"))
        hashAboutObj.insert("ZRSSI", hashAboutObj.value("ZRSSI").toString().split("_").first());

    if(hashAboutObj.contains("Type"))
        hashAboutObj.insert("Type", hashAboutObj.value("Type").toString().left(1));

    if(hashAboutObj.contains("CellID"))
        hashAboutObj.insert("CID", hashAboutObj.take("CellID"));

    if(hashAboutObj.contains("ZEUI64"))
        hashAboutObj.insert("EUI64", hashAboutObj.take("ZEUI64"));


    QStringList l;
    macL.clear();
    int fileNameLen = shaLen + 1;//sha1 len

    for(int i = 0; i < 10; i++){
        if(!hashAboutObj.value(QString("MAC%1").arg(i)).toString().isEmpty()){
            macL.append(hashAboutObj.value(QString("MAC%1").arg(i)).toString());
            l.append(QString("MAC%1:").arg(i) + macL.last().remove(":"));
            fileNameLen += l.last().length();
            fileNameLen++;
            if(fileNameLen > 250){//ext4 max file name len 255 byte
                l.takeLast();
                break;
            }
        }else
            break;
    }

    QStringList lk = QString("ID SN vrsn DEV app").split(" ");
    lk.append(QString("IMEI IMSI CID CellID LAC").split(" "));
    lk.append(QString("ZCH ZID EUI64").split(" "));

    lk.append(QString("RSSI RCSP ATI EcNo ZRSSI LQI VR HV Type").split(" "));//Low priority

    for(int i = 0, iMax = lk.size(); i < iMax; i++){
        if(!hashAboutObj.value(lk.at(i)).toString().isEmpty()){
            l.append(lk.at(i) + ":" + hashAboutObj.value(lk.at(i)).toString());
            fileNameLen += l.last().length();
            fileNameLen++;
            if(fileNameLen > 250){//ext4 max file name len 255 byte
                l.takeLast();
                break;
            }
        }
    }

    if(verboseMode)
        qDebug() << "fileNameFromAboutObject=" << l;
    return l.join("_");
}
//---------------------------------------------------------------------------------
