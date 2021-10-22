#include "m2mbackupconndecoder.h"

//-------------------------------------------------------------------------------------

M2MBackupConnDecoder::M2MBackupConnDecoder(const bool &verboseMode, QObject *parent) :
    DecodeMatildaProtocolBase(verboseMode, parent)
{
    myParams.msecCreated = QDateTime::currentMSecsSinceEpoch();
}

//-------------------------------------------------------------------------------------

bool M2MBackupConnDecoder::isCommandAllowed4thisConnectedDev(const quint16 &command, const bool &isQDSMode, const quint8 &accessLevel)
{
    if(accessLevel != MTD_USER_BACKUP ){
        //for unathorized devices only these two commands are allowed
         return ((isQDSMode && command == COMMAND_AUTHORIZE) || (!isQDSMode && command == COMMAND_ZULU));

    }

    if(command == COMMAND_I_AM_A_ZOMBIE)
        return true;

    if(isQDSMode){
        return (command == COMMAND_GET_CACHED_BACKUP);
    }
    return false;// (command == COMMAND_ZULU );
}

//-------------------------------------------------------------------------------------

void M2MBackupConnDecoder::setBackupParams(QByteArray write4authorizeBase64, QString lastSha1base64, QString workDir)
{
    myParams.auth = write4authorizeBase64;
    myParams.lastSha1Base64 = lastSha1base64;
    myParams.workDir = workDir;


}

//-------------------------------------------------------------------------------------

void M2MBackupConnDecoder::decodeReadDataJSON(QByteArray dataArr)
{
    if(lastObjSett.verboseMode)
        qDebug()  << "decodeReadDataJSON a0 " << connId.peerAddress << connId.socketDescriptor << dataArr;

    bool hshIsValid;
    quint16 command ;
    const QVariantHash hash = getHash4decodeReadDataJSON(dataArr, hshIsValid, command);

    if(lastObjSett.verboseMode)
        qDebug()  << "decodeReadDataJSON a2 " << connId.peerAddress << connId.socketDescriptor << hshIsValid << hash;

    if(!hshIsValid){
        speedStat.badByteReceived += dataArr.length();
        sendFunctionRezJSON(onCommandErrorLastOperationJSON(ERR_INCORRECT_REQUEST));
        return ;
    }
    dataArr.clear();

    bool doAfter = false;

    FunctionRezultJSON srezult;
    if(!isCommandAllowed4thisConnectedDev(command, false, lastObjSett.accessLevell)){
        qDebug() << "unknown command " << command << hash;
        ask2closeTheConnection(QString("unknown command ip: %1, command: %2").arg(connId.peerAddress).arg(command));
//        sendFunctionRezJSON(onCommandErrorLastOperationExtJSON(tr("Unknown command: %1. Ignoring...").arg(QString::number(command, 16)), ERR_INCORRECT_REQUEST));
        return;
    }
    srezult = getFunctionRezult4aCommandJSON(command, hash, doAfter);

    sendFunctionRezJSON(srezult);
    if(doAfter)
        onDoAfter(command);
}

//-------------------------------------------------------------------------------------

void M2MBackupConnDecoder::decodeReadData(const QVariant &dataVar, const quint16 &command)
{
    FunctionRezult srezult = FunctionRezult(QVariant(), command);
    bool doAfter = false;
    if(!isCommandAllowed4thisConnectedDev(command, true, lastObjSett.accessLevell)){

        if(lastObjSett.verboseMode)
            qDebug() << "unknown command isCommandAllowed4thisDev " << command << dataVar ;

        ask2closeTheConnection(QString("unknown command ip: %1, command: %2").arg(connId.peerAddress).arg(command));
        return;
//        return sendAfunctionRezult(onCommandAnswerNothing());

    }

    srezult = getFunctionRezult4aCommand(command, dataVar, doAfter);
    if(doAfter)
        onDoAfter(command);

}

//-------------------------------------------------------------------------------------

FunctionRezultJSON M2MBackupConnDecoder::getFunctionRezult4aCommandJSON(const quint16 &command, const QVariantHash &hash, bool &doAfter)
{
     switch(command){
     case COMMAND_ZULU          : return onCOMMAND_ZULU(hash, doAfter);
     case COMMAND_I_AM_A_ZOMBIE : return onCommandAnswerNothingJSON()                       ;// if(lastObjSett.verboseMode) qDebug() << "i am not a zombie json" << connId.socketDescriptor << connId.peerAddress; break;}

     }

     //         qDebug() << "unknown command " << command << hash << connId.peerAddress << connId.otherId;
     emit addError2Log(QString("unknown command ip: %1, command: %2").arg(connId.peerAddress).arg(command));

     doAfter = true;
     return onCommandAnswerNothingJSON();
     // onCommandErrorLastOperationExtJSON(tr("Unknown command: %1. Ignoring...").arg(QString::number(command, 16)), ERR_INCORRECT_REQUEST);

}

//-------------------------------------------------------------------------------------

FunctionRezultJSON M2MBackupConnDecoder::onCOMMAND_ZULU(const QVariantHash &hash, bool &doAfter)
{
    if(hash.value("name").toString() == "Matilda" && hash.value("version").toInt() > 0 && QDateTime::fromString(hash.value("UTC").toString(), "yyyy-MM-dd hh:mm:ss").isValid()){

        const int qds = hash.value("QDS").toInt();//QDataStream::Qt_5_6 must be used

        if(hash.value("err").toString().isEmpty() && qds >= QDataStream::Qt_4_8 && qds <= QDataStream::Qt_DefaultCompiledVersion){

//            lastObjSett.useJsonMode = true;
//            matildaLogined = true;
//             dataStreamVersion = hash.value("QDS").toInt();

            QJsonObject jObj;
            jObj.insert("version", hash.value("version").toInt());
            jObj.insert("hsh", QString(myParams.auth));// write4authorizeBase64);// QString(QCryptographicHash::hash(loginPasswd.at(0) + "\n" + dataArr + "\n" + loginPasswd.at(1), QCryptographicHash::Sha3_256).toBase64()));
            //
            //mode JSON and QDataStream
            jObj.insert("QDS", QString::number(qds));// dataStreamVersion));//активація режиму QDataStream
            jObj.insert("cmmprssn", "zlib");
            jObj.insert("pos", "up");//only 4 backup
//            stopAll = false;

//             mWrite2SocketJSON(jObj, COMMAND_AUTHORIZE, 2);
            return FunctionRezultJSON(jObj, COMMAND_AUTHORIZE);
        }
    }
    doAfter = true;
    return onCommandAnswerNothingJSON();
}

//-------------------------------------------------------------------------------------

FunctionRezult M2MBackupConnDecoder::getFunctionRezult4aCommand(const quint16 &command, const QVariant &dataVar, bool &doAfter)
{
    lastDecodeAllowedCommand = command;
    switch (command) {
    case COMMAND_I_AM_A_ZOMBIE      : return onCOMMAND_I_AM_A_ZOMBIE()                      ;
    case COMMAND_AUTHORIZE          : return onCOMMAND_AUTHORIZE(dataVar, doAfter);
    case COMMAND_GET_CACHED_BACKUP  : return onCOMMAND_GET_CACHED_BACKUP(dataVar, doAfter)  ;
    }

    emit addError2Log(QString("unknown command ip: %1, command: %2, useThisProtocolVersion=%3").arg(connId.peerAddress).arg(command).arg(useThisProtocolVersion));
    doAfter = true;
    return onCommandAnswerNothing();//  onCommandErrorLastOperationExt(tr("Unknown command: %1. Ignoring...").arg(QString::number(command, 16)), ERR_INCORRECT_REQUEST);
}

//-------------------------------------------------------------------------------------

FunctionRezult M2MBackupConnDecoder::onCOMMAND_AUTHORIZE(const QVariant &dataVar, bool &doAfter)
{
    if(lastObjSett.verboseMode)  qDebug() << "Socket4uploadBackup access = " << dataVar.toHash();

    const QVariantHash h = dataVar.toHash();

    lastObjSett.accessLevell = h.value("a").toUInt();
    const QByteArray serviceAccessKey = getServiceAccessKey(myParams.auth);
//    QCryptographicHash::hash("$Try2Annet$\t\n\r "+ write4authorizeBase64.toLocal8Bit() + " \r\n\t$Try2Annet$",
//                                                   #if QT_VERSION >= 0x050902
//                                                               QCryptographicHash::Keccak_256
//                                                   #else
//                                                               QCryptographicHash::Sha3_256
//                                                   #endif
//                                                           );

    if(lastObjSett.accessLevell == MTD_USER_BACKUP && serviceAccessKey == h.value("sak").toByteArray()){

        //make preparation
        myParams.hashAboutObj = h.value("ao").toHash();
        myParams.backupArr.clear();
        myParams.backupArrLen = 0;

        return FunctionRezult(QVariantHash(), COMMAND_GET_CACHED_BACKUP);//start read
    }
    lastObjSett.accessLevell = MTD_USER_NOAUTH;
    doAfter = true;
    return onCommandAnswerNothing();
//    return onCommandErrorLastOperationExt(QString("bad access level"), ERR_ACCESS_DENIED);
//    ask2closeTheConnection(QString("bad access level"));


}

//-------------------------------------------------------------------------------------

FunctionRezult M2MBackupConnDecoder::onCOMMAND_I_AM_A_ZOMBIE()
{
    if(lastObjSett.verboseMode) qDebug() << "i am not a zombie uc" << connId.peerAddress << connId.socketDescriptor << connId.otherId;
    return onCommandAnswerNothing();// FunctionRezult(QVariant());
}

//-------------------------------------------------------------------------------------

FunctionRezult M2MBackupConnDecoder::onCOMMAND_GET_CACHED_BACKUP(const QVariant &dataVar, bool &doAfter)
{

    const QVariantHash h = dataVar.toHash();
    if(h.contains("t"))
        myParams.backupArrLen = h.value("t").toInt();
    const qint32 pos = h.value("i").toInt();

    myParams.backupArr.append(h.value("d").toByteArray());
    if(pos < 0 || !h.contains("d")){
        //закінчено зчитування
        if(myParams.backupArr.length() == myParams.backupArrLen && myParams.backupArrLen > 0){
            saveBackupArrAsFile();
        }
        doAfter = true;
        return onCommandAnswerNothing();
    }
    QVariantHash hh;
    hh.insert("i", pos);
    return FunctionRezult(hh, COMMAND_GET_CACHED_BACKUP);
}

//-------------------------------------------------------------------------------------

void M2MBackupConnDecoder::saveBackupArrAsFile()
{
    if(myParams.lastSha1Base64.isEmpty())
        myParams.lastSha1Base64 = QCryptographicHash::hash(myParams.backupArr, QCryptographicHash::Sha1).toBase64(QByteArray::OmitTrailingEquals);// toHex().toLower();// toBase64(QByteArray::OmitTrailingEquals);
    if(myParams.lastSha1Base64.isEmpty() || myParams.backupArr.isEmpty()){
        if(lastObjSett.verboseMode)
            qDebug() << "noSha1=" << myParams.lastSha1Base64.isEmpty() << ", noData=" << myParams.backupArr.isEmpty() << ", dataLen=" << myParams.backupArrLen;
        return;
    }

    //create new backup
    //<work dir>/<year>/<month>/<file names>  //UTC date time!!!
    //file name <base64 ( omit "=",  replace("/", "=", on file only)) sha1>_<mac(X)>_...other keys

//    QString workDir = SettLoader4svaha().loadOneSett(SETT_SYNC_WORKDIR).toString();
    if(myParams.workDir.isEmpty()){
        if(lastObjSett.verboseMode)
            qDebug() << "workDir is not valid " << myParams.workDir;
        return;
    }

    const QDateTime dtCreatedBackupUtc = QDateTime::fromMSecsSinceEpoch(myParams.msecCreated);

    const QString path2dir = QString("%1/%2/%3").arg(myParams.workDir).arg(dtCreatedBackupUtc.date().year()).arg(dtCreatedBackupUtc.date().month());
    QDir dir(path2dir);
    if(!dir.exists())
        dir.mkpath(path2dir);

    QStringList macL;
    QString fileName = myParams.lastSha1Base64 + "_" + fileNameFromAboutObject(macL, myParams.lastSha1Base64.length());
    fileName = fileName.replace("/", "=");

    QSaveFile sFile(QString("%1/%2").arg(path2dir).arg(fileName));
    if(sFile.open(QSaveFile::WriteOnly|QSaveFile::Unbuffered)){
        sFile.write(myParams.backupArr);
        if(sFile.commit()){
            if(lastObjSett.verboseMode)
                qDebug() << "backup saved " << path2dir << fileName ;

            emit syncDone(macL, myParams.lastSha1Base64, myParams.msecCreated);
            return;
        }
        if(lastObjSett.verboseMode)
            qDebug() << "can't save file " << path2dir << fileName << sFile.errorString();
        emit addError2Log(QString("Failed to save a backup file %1").arg(sFile.errorString()));
        return;
    }



    emit addError2Log(QString("Failed to create a backup file %1").arg(sFile.errorString()));
    if(lastObjSett.verboseMode)
        qDebug() << "Socket4uploadBackup can't creat file " << fileName << sFile.errorString();



}

//-------------------------------------------------------------------------------------

QString M2MBackupConnDecoder::fileNameFromAboutObject(QStringList &macL, const int &shaLen)
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

    QVariantHash hashAboutObj = myParams.hashAboutObj;
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

    if(lastObjSett.verboseMode)
        qDebug() << "fileNameFromAboutObject=" << l;
    return l.join("_");
}

//-------------------------------------------------------------------------------------

void M2MBackupConnDecoder::onDoAfter(const quint16 &command)
{
    ask2closeTheConnection(QString("it has to be closed after the command %1, ip %2, id %3")
                           .arg(command).arg(connId.peerAddress).arg(connId.socketDescriptor));
}

void M2MBackupConnDecoder::restartTimeObject()
{
    timeObjectSmpl.restart();
}

//-------------------------------------------------------------------------------------
