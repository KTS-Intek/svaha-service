#include "m2mconnholderdecoder.h"



//---------------------------------------------------------------------------------------

M2MConnHolderDecoder::M2MConnHolderDecoder(const QHostAddress &peerAddress, const qintptr &socketDescriptor, const bool &verboseMode, QObject *parent)
    : DecodeMatildaProtocolBase(verboseMode, parent)
{
    connId.socketDescriptor = QString::number(socketDescriptor);
    connId.peerAddress = NetworkConvertHelper::showNormalIP(peerAddress);

    restartTimeObject();


}



bool M2MConnHolderDecoder::isCommandAllowed4thisConnectedDev(const quint16 &command, const qint8 &connectedDevType)
{
    bool r = false;
    switch(connectedDevType){
    case REM_DEV_MATILDA_UNKNWN : r = (command == COMMAND_YOUR_ID_AND_MAC || command == COMMAND_CONNECT_ME_2_THIS_ID_OR_MAC)          ; break; //активний сокет від пристрою опитування, один раз передає свої дані реєстарції
    case REM_DEV_MATILDA_DEV    : r = getCommand4remDevMatilda().contains(command)  ; break;
    case REM_DEV_MATILDA_CONF   : r = getCommand4remDevUCon().contains(command)     ; break;
    }
    return r;
}

//---------------------------------------------------------------------------------------

QList<quint16> M2MConnHolderDecoder::getCommand4remDevMatilda()
{
    return QList<quint16>()
            << COMMAND_I_AM_A_ZOMBIE

            << COMMAND_CHECK_BACKUP_FILE_HASH_SUMM;
    ;
}

//---------------------------------------------------------------------------------------

QList<quint16> M2MConnHolderDecoder::getCommand4remDevUCon()
{
    return QList<quint16>()
            << COMMAND_I_AM_A_ZOMBIE

            << COMMAND_CONNECT_ME_2_THIS_ID_OR_MAC //запит від ПЗ конфігурації на з’єднання

               ;
}

QString M2MConnHolderDecoder::getRemoteIpAndDescr()
{
    return QString("%1:%2")
            .arg(connId.peerAddress, connId.socketDescriptor);
}

QStringHash M2MConnHolderDecoder::getObjIfo(const QVariantMap &h, const bool &addVersion)
{
    QStringHash hashAboutObj;
    if(!h.isEmpty()){
        QStringList lk = QString("SN vrsn DEV app IMEI IMSI CellID LAC RSSI RCSP ATI Ec/No ZCH ZID ZRSSI LQI VR HV Type EUI64 ZEUI64").split(" ");
        for(int i = 0, iMax = lk.size(); i < iMax; i++){
            if(h.contains(lk.at(i)) && !h.value(lk.at(i)).toString().isEmpty())
                hashAboutObj.insert(lk.at(i), h.value(lk.at(i)).toString());
        }
        if(hashAboutObj.contains("ZEUI64"))
            hashAboutObj.insert("EUI64", hashAboutObj.value("ZEUI64"));
    }else{
        if(addVersion)
            hashAboutObj.insert("vrsn", QString::number(MATILDA_PROTOCOL_VERSION_V1));
    }
    //    lastAboutObj = hashAboutObj;

    if(lastObjSett.verboseMode)
        qDebug() << "hashAboutObj " << hashAboutObj << h;
    return hashAboutObj;
}

//---------------------------------------------------------------------------------------

bool M2MConnHolderDecoder::areServerIpPortValidAndMySocketID(const QString &serverIp, const quint16 &serverPort, const QString &objSocketId)
{
    return (isMySocketID(objSocketId) && !serverIp.isEmpty() && serverPort > 1000 && serverPort < 65535);
}

//---------------------------------------------------------------------------------------

bool M2MConnHolderDecoder::isMySocketID(const QString &objSocketId)
{
    return (!objSocketId.isEmpty() && objSocketId == connId.otherId);
}

bool M2MConnHolderDecoder::isMySocketIDinTheList(const QStringList &objSocketIds)
{
    return objSocketIds.contains(connId.otherId);

}

//---------------------------------------------------------------------------------------

void M2MConnHolderDecoder::decodeReadDataJSON(QByteArray dataArr)
{
    restartZombieTmr();

    if(lastObjSett.verboseMode)
        qDebug()  << "decodeReadDataJSON a0 " <<  connId.otherId << dataArr;

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
    if(!listUsedCommands.contains(command)){
        if(!isCommandAllowed4thisConnectedDev(command, myStateParams.connectedDevType)){
            qDebug() << "unknown command " << command << hash;
            emit addError2Log(QString("unknown command ip: %1, command: %2").arg(connId.peerAddress).arg(command));
            sendFunctionRezJSON(onCommandErrorLastOperationExtJSON(tr("Unknown command: %1. Ignoring...").arg(QString::number(command, 16)), ERR_INCORRECT_REQUEST));
            return;
        }
        listUsedCommands.append(command);
    }



    srezult = getFunctionRezult4aCommandJSON(command, hash, doAfter);

    sendFunctionRezJSON(srezult);
    if(doAfter)
        onDoAfter(command);


}


//---------------------------------------------------------------------------------------
FunctionRezultJSON M2MConnHolderDecoder::getFunctionRezult4aCommandJSON(const quint16 &command, const QVariantHash &hash, bool &doAfter)
{
    switch(command){
    case COMMAND_YOUR_ID_AND_MAC            : return onCOMMAND_YOUR_ID_AND_MAC(hash, doAfter)            ;
    case COMMAND_I_AM_A_ZOMBIE              : return onCOMMAND_I_AM_A_ZOMBIE(hash)                        ;
    case COMMAND_CONNECT_ME_2_THIS_ID_OR_MAC: return onCOMMAND_CONNECT_ME_2_THIS_ID_OR_MAC(hash, doAfter);
    case COMMAND_CHECK_BACKUP_FILE_HASH_SUMM: return onCOMMAND_CHECK_BACKUP_FILE_HASH_SUMM(hash, doAfter);
    default: qDebug() << "unknown command " << command; doAfter = true; break;

    }

    return onCommandAnswerNothingJSON();


}

//---------------------------------------------------------------------------------------

FunctionRezultJSON M2MConnHolderDecoder::onCOMMAND_YOUR_ID_AND_MAC(const QVariantHash &hash, bool &doAfter)
{
    //активний сокет від пристрою опитування, один раз передає свої дані реєстарції
    myStateParams.connectedDevType = REM_DEV_MATILDA_DEV;

    myStateParams.mIden = hash.value("memo").toString();
    myStateParams.mMac.clear();
    const QStringList macList = hash.value("mac").toString().toUpper().split("|", QString::SkipEmptyParts);

    for(int i = 0, iMax = macList.size(), j = 0; i < iMax && j < 10; i++){
        if(macList.at(i).startsWith("00:00:00:00:") ||
                macList.at(i).toLower().startsWith("ff:ff:ff:ff") ||
                !macList.at(i).contains(":") ||
                myStateParams.mMac.contains(macList.at(i)))
            continue;
        myStateParams.mMac.append(macList.at(i));
        j++;
    }

    if(myStateParams.mMac.isEmpty() || hash.value("mac").toString().remove("|").isEmpty()){
        if(lastObjSett.verboseMode)
            qDebug() << "bad mac " << connId.peerAddress << connId.socketDescriptor << hash;

        doAfter = true; //close the connection later
        return onCommandErrorLastOperationExtJSON(tr("MAC list is empty"), ERR_INCORRECT_REQUEST);
    }

    if(hash.value("cmprssn").toString().contains("zlib"))
        lastObjSett.allowCompress = true;

    connId.otherId = getRemoteIpAndDescr();

    if(connId.peerAddress.isEmpty()){
        emit addError2Log(QString("empty peerAddress"));
        if(lastObjSett.verboseMode)
            qDebug() << "onCOMMAND_YOUR_ID_AND_MAC " << connId.peerAddress << connId.socketDescriptor << connId.nameStr;
    }

    emit removeThisIpFromTemporaryBlockList(connId.peerAddress);

    emit addMyId2Hash(myStateParams.mIden, myStateParams.mMac, connId.otherId, getObjIfo(hash.value("ao").toMap(), true), hash.contains("ao"));
    //    emit writeThisDataJSON(errCodeLastOperationJson(command, ERR_NO_ERROR), COMMAND_ERROR_CODE);//реєстрацію завершено упішно
    const int protocol = hash.value("ao").toHash().value("vrsn", MATILDA_PROTOCOL_VERSION_V1).toInt();
    if(protocol > 0)
        useThisProtocolVersion = protocol;

    myStateParams.ucDeviceType = hash.value("ao").toHash().value("DEV", 0).toUInt();
    //1 - UC - supports backups if protocol > 1
    //4 - M2M service


    return onCommandErrorLastOperationNoErrorJSON();

}

//---------------------------------------------------------------------------------------

FunctionRezultJSON M2MConnHolderDecoder::onCOMMAND_I_AM_A_ZOMBIE(const QVariantHash &hash)
{
    //    if(myStateParams.connectedDevType == REM_DEV_MATILDA_DEV || myStateParams.connectedDevType == REM_DEV_MATILDA_CONF){
    if(lastObjSett.verboseMode)
        qDebug() << "zombie killer" << connId.peerAddress << myStateParams.mIden << myZombieKiller.timeZombie.elapsed() / 1000;
    if(hash.contains("ao"))//періодично мені відправляються дані по пристрою
        emit setInfoAboutObj(myStateParams.mMac, getObjIfo(hash.value("ao").toMap(), false), myStateParams.mMac.size());


    if(myZombieKiller.timeZombie.elapsed() > 5000){
        myZombieKiller.timeZombie.restart();
        QJsonObject j;
        j.insert("msec", QDateTime::currentMSecsSinceEpoch());
        return FunctionRezultJSON(j, COMMAND_I_AM_A_ZOMBIE);
    }

    return onCommandAnswerNothingJSON();
    //        emit writeThisDataJSON(QJsonObject(), COMMAND_I_AM_A_ZOMBIE);
}

//---------------------------------------------------------------------------------------

FunctionRezultJSON M2MConnHolderDecoder::onCOMMAND_CONNECT_ME_2_THIS_ID_OR_MAC(const QVariantHash &hash, bool &doAfter)
{

    //from UCon
    //    if(myStateParams.connectedDevType == REM_DEV_MATILDA_DEV){
    //        emit writeThisDataJSON(errCodeLastOperationJson(command, ERR_INCORRECT_REQUEST), COMMAND_ERROR_CODE);
    //        return;
    //    }
    myStateParams.connectedDevType = REM_DEV_MATILDA_CONF;
    connId.otherId = getRemoteIpAndDescr();


    bool isIDMode = hash.value("useId", false).toBool();
    QString str = hash.value("remote", "").toString();

    if(lastObjSett.verboseMode)
        qDebug() << "COMMAND_CONNECT_ME_2_THIS_ID_OR_MAC " << isIDMode << str;

    if(str.isEmpty()){
        doAfter = true;
        return onCommandErrorLastOperationExtJSON(QString("'remote' is empty"), ERR_INCORRECT_REQUEST);
        //        emit writeThisDataJSON(errCodeLastOperationJson(command, ERR_INCORRECT_REQUEST), COMMAND_ERROR_CODE);
    }
    //    void connMe2ThisIdOrMac(QString macOrId, bool isMac, QString myRemoteId, QString rIp);//mac or id, isMacMode, socket id

    emit removeThisIpFromTemporaryBlockList(connId.peerAddress);

    emit connMe2ThisIdOrMac(str, !isIDMode, connId.otherId, connId.peerAddress);
    return onCommandAnswerNothingJSON();
}

//---------------------------------------------------------------------------------------

FunctionRezultJSON M2MConnHolderDecoder::onCOMMAND_CHECK_BACKUP_FILE_HASH_SUMM(const QVariantHash &hash, bool &doAfter)
{
    Q_UNUSED(doAfter);
    //with UC
    if(lastObjSett.verboseMode)
        qDebug() << "COMMAND_CHECK_BACKUP_FILE_HASH_SUMM " << myStateParams.connectedDevType << hash;

    //    if(myStateParams.connectedDevType != REM_DEV_MATILDA_DEV){
    //        emit writeThisDataJSON(errCodeLastOperationJson(command, ERR_INCORRECT_REQUEST), COMMAND_ERROR_CODE);
    //        myStateParams.connectedDevType = REM_DEV_MATILDA_UNKNWN;
    //        QTimer::singleShot(5555, this, SLOT(onDisconn()) );
    //        break;
    //    }


    if(hash.value("hshChngd", true).toBool()){
        emit setInfoAboutObj(myStateParams.mMac, getObjIfo(hash.value("ao").toMap(), false), myStateParams.mMac.size());


        const quint16 svahaPort = startUploadBackup( hash.value("hsh").toString());
        if(lastObjSett.verboseMode)
            qDebug() << "hshChngd= " << hash.value("hsh").toString() << hash.value("ssn").toInt() << myStateParams.lastSha1base64 << svahaPort;

        if(svahaPort != 0){

            addLine2log(tr("Configuration has changed, backup process is starting..."));

            //            if(myStateParams.connectedDevType == REM_DEV_MATILDA_DEV){
            QJsonObject jObj;//створити з'єднання в режимі вивантаження резервної копії
            jObj.insert("sIp", myStateParams.serverDataIP);// serverServiceIP);
            jObj.insert("sP", svahaPort);
            jObj.insert("rIp", hash.value("hsh").toString());
            jObj.insert("ssn", hash.value("ssn").toInt());

            return FunctionRezultJSON(jObj, COMMAND_CONNECT_2_THIS_SERVICE);
            //                emit writeThisDataJSON(jObj, COMMAND_CONNECT_2_THIS_SERVICE);
            //                return;

        }
    }
    //конфігурація залишилась без змін,
    addLine2log(tr("Configuration is the same"));
    emit onSyncRequestRemoteSha1isEqual(myStateParams.macL4backupManager);
    myStateParams.dtLastBackupCheck = QDateTime::currentDateTimeUtc();

    myStateParams.macL4backupManager.clear();
    return onCommandAnswerNothingJSON();

}

//---------------------------------------------------------------------------------------

quint16 M2MConnHolderDecoder::startUploadBackup(const QString &lastSha1base64)
{
    myStateParams.backupSessionId++;
    myStateParams.lastBackupServerPort = 0;

    if(myStateParams.serverDataIP.isEmpty()){
        //        myStateParams.serverIP = "kts-m2m.ddns.net";
        //        myStateParams.serverPort = 65000;
        return 0;//nothing will work
    }


    if(myStateParams.serverDataStart > 65534 || myStateParams.serverDataEnd > 65534 || myStateParams.serverDataStart < 1000 || myStateParams.serverDataEnd < 1000){
        //        myStateParams.serverDataStart = SettLoader4svaha::defSETT_SVAHA_DATA_START_PORT();
        //        myStateParams.serverDataEnd = myStateParams.serverDataStart + SettLoader4svaha::defSETT_SVAHA_DATA_PORT_COUNT();
        return 0;
    }
    myStateParams.lastSha1base64 = lastSha1base64;

    emit createBackupReceiver(myStateParams.workDir, myStateParams.serverDataStart, myStateParams.serverDataEnd);

    //you have to call getBackupSign, from your connection holder


    return myStateParams.lastBackupServerPort;
}

//---------------------------------------------------------------------------------------

QByteArray M2MConnHolderDecoder::getBackupSign(const quint16 &serverPort)
{
    myStateParams.lastBackupServerPort = serverPort;


    const QByteArray getSign = getBackupServiceAccessKey(serverPort, myStateParams.serverDataIP, myStateParams.lastSha1base64);
    //    QCryptographicHash::hash(QString("%1\n\t%2\n\t%3\n\tAnnet\n\t")
    //                                                        .arg(QString::number(serverPort))
    //                                                        .arg(myStateParams.serverIP).arg(myStateParams.lastSha1base64).toLocal8Bit(),
    //                                                    #if QT_VERSION >= 0x050902
    //                                                                QCryptographicHash::Keccak_256
    //                                                    #else
    //                                                                QCryptographicHash::Sha3_256
    //                                                    #endif
    //                                                            ).toBase64(QByteArray::OmitTrailingEquals);

    return getSign;
}

//---------------------------------------------------------------------------------------

void M2MConnHolderDecoder::onDoAfter(const quint16 &command)
{
    ask2closeTheConnection(QString("bad command %1, ip %2, id %3")
                           .arg(command).arg(connId.peerAddress, connId.socketDescriptor));
}

void M2MConnHolderDecoder::createZombieTmr(const int &zombieMsec)
{
    myZombieKiller.timeZombie.start();

    setZombieMsec(zombieMsec);

    QTimer *zombieTmr = new QTimer(this);
    zombieTmr->setSingleShot(true);
    zombieTmr->setInterval( myZombieKiller.zombieMsec);// 15 * 60 * 1000 );

    connect(zombieTmr, &QTimer::timeout, this, &M2MConnHolderDecoder::checkSendZombieCommand);
    connect(this, &M2MConnHolderDecoder::disconnLater, zombieTmr, &QTimer::stop);
    connect(this, SIGNAL(startTmrZombieKiller(int)), zombieTmr, SLOT(start(int)));


    QTimer *suicideTmr = new QTimer(this);
    suicideTmr->setSingleShot(true);
    suicideTmr->setInterval(33333);

    connect(suicideTmr, &QTimer::timeout, this, &M2MConnHolderDecoder::startSuicide);
    connect(this, &M2MConnHolderDecoder::onM2MServerAcceptedMe, suicideTmr, &QTimer::stop);
    connect(this, &M2MConnHolderDecoder::killTemporaryObjects, suicideTmr, &QTimer::deleteLater);


    restartZombieTmr();


}

//---------------------------------------------------------------------------------------

void M2MConnHolderDecoder::startConnMatildaDev(quint16 serverPort, QString objId, QString objMac, QString objSocketId, QString rIp)
{
    if(myStateParams.connectedDevType == REM_DEV_MATILDA_DEV){
        if(objId == myStateParams.mIden && myStateParams.mMac.contains(objMac) && areServerIpPortValidAndMySocketID(myStateParams.serverDataIP, serverPort, objSocketId)){
            QJsonObject jObj;
            jObj.insert("sIp", myStateParams.serverDataIP);
            jObj.insert("sP", serverPort);
            jObj.insert("rIp", rIp);
            emit writeThisDataJSON(jObj, COMMAND_CONNECT_2_THIS_SERVICE);

            //is the socket alive, fast check must be done

            fastZombieCheckSmart();
        }
    }
}

//---------------------------------------------------------------------------------------

void M2MConnHolderDecoder::startConn4UCon(quint16 serverPort, QString objSocketId)
{
    if(myStateParams.connectedDevType == REM_DEV_MATILDA_CONF){
        if(areServerIpPortValidAndMySocketID(myStateParams.serverDataIP, serverPort, objSocketId)){
            QJsonObject jObj;
            jObj.insert("sIp", myStateParams.serverDataIP);// serverIp);
            jObj.insert("sP", serverPort);
            emit writeThisDataJSON(jObj, COMMAND_CONNECT_ME_2_THIS_ID_OR_MAC);

            ask2closeTheConnectionExt(QString("startConn4UCon"), 1234);
        }
    }
}

//---------------------------------------------------------------------------------------

void M2MConnHolderDecoder::onFailed2connect2oneDev(QStringList macIdList, QString socketId)
{
    if(isMySocketID(socketId)){
        QJsonObject jObj;
        if(macIdList.isEmpty()){
            jObj.insert("lcmd", COMMAND_CONNECT_ME_2_THIS_ID_OR_MAC);
            jObj.insert("e", ERR_RESOURCE_BUSY);
            jObj.insert("em", tr("Unknown device"));
            emit writeThisDataJSON(jObj, COMMAND_ERROR_CODE_EXT);

        }else{
            jObj.insert(QString("l"), QJsonArray::fromStringList(macIdList));
            emit writeThisDataJSON(jObj, COMMAND_CONNECT_ME_2_THIS_ID_OR_MAC);
        }
        //        jObj.insert("sIp", "");
        //        jObj.insert("sp", "");

    }
}

//---------------------------------------------------------------------------------------

void M2MConnHolderDecoder::onResourBusy(QString socketId)
{
    if(isMySocketID(socketId)){
        sendFunctionRezJSON(onCommandErrorLastOperationJSON(ERR_RESOURCE_BUSY));

        //       emit writeThisDataJSON( errCodeLastOperationJson()// errCodeLastOperationJson(lastCommand, ERR_RESOURCE_BUSY), COMMAND_ERROR_CODE);//
    }
}

void M2MConnHolderDecoder::checkThisMac(QString mac)
{


    if(myStateParams.connectedDevType == REM_DEV_MATILDA_DEV && myStateParams.mMac.contains(mac)){
        //        emit addMyId2Hash(mIden, QStringList() << mac, myRemoteIpAndDescr);

        if(connId.stopAll){
            addLine2log(QString("checkThisMac stopped %1 %2").arg(connId.otherId, mac));

            return;
        }

        ask2closeTheConnection(QString("checkThisMac %1 %2").arg(connId.otherId, mac));
    }


}

//void M2MConnHolderDecoder::killClientNow(QString id, bool byDevId)
//{
//    if((byDevId && id == myStateParams.mIden) || (!byDevId && isMySocketID(id)))
//        ask2closeTheConnection(QString("External command"));
//}

void M2MConnHolderDecoder::killClientsNow(QStringList ids, bool byDevId)
{
    if((byDevId && ids.contains(myStateParams.mIden)) || (!byDevId && isMySocketIDinTheList(ids))){
        ask2closeTheConnection(QString("External command"));
    }

}

void M2MConnHolderDecoder::checkBackup4thisMac(QString mac, QString lastSha1base64)
{
    //it comes from backup manager
    if(myStateParams.mMac.contains(mac) && !myStateParams.macL4backupManager.contains(mac)){


        const QDateTime dt = QDateTime::currentDateTimeUtc();


        if(!myStateParams.dtLastBackupCheck.isValid()){

            if(myStateParams.ucDeviceType == DEV_POLL && useThisProtocolVersion > MATILDA_PROTOCOL_VERSION_V1){

                //it is here for the first time
                myStateParams.dtLastBackupCheck = dt;
                QTimer *t = new QTimer(this);
                t->setSingleShot(true);
                t->setInterval(111);

                connect(this, SIGNAL(startTmrCheckRemoteSha1()), t, SLOT(start()) );
                connect(this, SIGNAL(closeTheConnection()), t, SLOT(stop()) );
                connect(t, SIGNAL(timeout()), this, SLOT(checkRemoteSha1()) );

            }else{
                myStateParams.macL4backupManager.append(mac);
                //backup feature doesn't work
            }

        }else{
            if(myStateParams.dtLastBackupCheck > dt){
                //зараз виконуються операції з вивантаження файлу
                myStateParams.macL4backupManager.append(mac);
                return;
            }
            myStateParams.dtLastBackupCheck = dt;
        }

        myStateParams.lastSha1base64 = lastSha1base64;
        myStateParams.macL4backupManager.append(mac);


        emit startTmrCheckRemoteSha1();

        if(lastObjSett.verboseMode)
            qDebug() << "checkBackup4thisMac " << mac << lastSha1base64 << myStateParams.dtLastBackupCheck << dt;

    }

}

void M2MConnHolderDecoder::onSyncDone(quint8 backupSessionId, QStringList macL, QString lastSha1base64, qint64 msecCreated)
{
    if(lastObjSett.verboseMode)
        qDebug() << "SocketDlyaTrymacha::onSyncDone "
                 << myStateParams.backupSessionId << backupSessionId << lastSha1base64
                 << QDateTime::fromMSecsSinceEpoch(msecCreated)
                 << myStateParams.lastSha1base64 << myStateParams.macL4backupManager;

    if(backupSessionId == myStateParams.backupSessionId) {
        myStateParams.macL4backupManager.append(macL);
        myStateParams.macL4backupManager.removeDuplicates();

        emit onSyncFileDownloaded(myStateParams.macL4backupManager, lastSha1base64, msecCreated);

        myStateParams.lastSha1base64 = lastSha1base64;
        myStateParams.macL4backupManager.clear();
        myStateParams.dtLastBackupCheck = QDateTime::currentDateTimeUtc();
    }
}

void M2MConnHolderDecoder::onSyncServiceDestr(quint8 backupSessionId)
{
    if(lastObjSett.verboseMode)
        qDebug() << "SocketDlyaTrymacha::onSyncServiceDestr " << myStateParams.backupSessionId
                 << backupSessionId << myStateParams.lastSha1base64 << myStateParams.macL4backupManager;

    if(myStateParams.backupSessionId == backupSessionId) {
        if(!myStateParams.macL4backupManager.isEmpty())
            emit onSyncRequestRemoteSha1isEqual(myStateParams.macL4backupManager);

        myStateParams.macL4backupManager.clear();
        myStateParams.dtLastBackupCheck = QDateTime::currentDateTimeUtc();
    }
}

//---------------------------------------------------------------------------------------

void M2MConnHolderDecoder::restartTimeObject()
{
    timeObjectSmpl.restart();

}


//---------------------------------------------------------------------------------------

void M2MConnHolderDecoder::sendLastConnectionState()
{
    if(theconnection.conntype.isEmpty())
        return;

    emit setServerInConnIdExtData(theconnection.conntype, theconnection.connid, theconnection.msecstart,
                                  theconnection.msecend, theconnection.rb, theconnection.wb, theconnection.lastmessage);

}

//---------------------------------------------------------------------------------------

void M2MConnHolderDecoder::checkRemoteSha1()
{
    if(!myStateParams.macL4backupManager.isEmpty()){
        myStateParams.dtLastBackupCheck = QDateTime::currentDateTimeUtc().addSecs(12 * 60 * 60);
        QJsonObject j;
        j.insert("lHsh", myStateParams.lastSha1base64);

        emit writeThisDataJSON(j, COMMAND_CHECK_BACKUP_FILE_HASH_SUMM);

    }
}

//---------------------------------------------------------------------------------------

void M2MConnHolderDecoder::onDataConnectionParamsChanged(QString serverDataIP, quint16 serverDataStart, quint16 serverDataEnd)
{
    //    QString serverServiceIP;
    //    quint16 serverServicePort;//65000

    //    //service - data exchange
    //    QString serverDataIP;
    //    quint16 serverPort;
    //    quint16 serverDataStart;
    //    quint16 serverDataEnd;
    //    myStateParams.serverServiceIP = serverAddr;
    //    myStateParams.serverServicePort = serverServicePort;

    myStateParams.serverDataIP = serverDataIP;

    //    myStateParams.serverPort = serverPort;
    myStateParams.serverDataStart = serverDataStart;
    myStateParams.serverDataEnd = serverDataEnd;
}

//---------------------------------------------------------------------------------------

void M2MConnHolderDecoder::onBackupWorkDirectoryChanged(QString workDir)
{
    myStateParams.workDir = workDir;
}

//---------------------------------------------------------------------------------------

void M2MConnHolderDecoder::onEverythingIsConnected()
{
    QTimer::singleShot(111, this, SIGNAL(onForceReading()));
}

void M2MConnHolderDecoder::setZombieMsec(int msec)
{
    myZombieKiller.zombieMsec = quint32(msec);


    if(myZombieKiller.zombieMsec < 20000)
        myZombieKiller.zombieMsec = 20000;
    else if(myZombieKiller.zombieMsec > 600000) //maximum
        myZombieKiller.zombieMsec = 600000;

}

//---------------------------------------------------------------------------------------

void M2MConnHolderDecoder::checkSendZombieCommand()
{

    if(connId.stopAll){
        addLine2log(tr("checkSendZombieCommand is stopped"));
//        emit disconnLater(1);//ASAP
        emit closeTheConnection();
        return;
    }

    if(myZombieKiller.isWaiting4answer){
        ask2closeTheConnectionExt("checkSendZombieCommand no answer ", 11);
    }else{

        myZombieKiller.isWaiting4answer = true;

        if(useThisProtocolVersion >= MATILDA_PROTOCOL_VERSION_V11){
            myZombieKiller.timeZombie.restart();
            QJsonObject j;
            j.insert("msec", QDateTime::currentMSecsSinceEpoch());
            j.insert("fast", true);
            //        return FunctionRezultJSON(j, COMMAND_I_AM_A_ZOMBIE);

            emit writeThisDataJSON(j, COMMAND_I_AM_A_ZOMBIE);

            emit startTmrZombieKiller(10000);
            return;//older don't support this feature

        }
        emit startTmrZombieKiller(myZombieKiller.zombieMsec);

    }

}

void M2MConnHolderDecoder::restartZombieTmr()
{
    //something is received

    restartZombieTmrExt(false);

}

void M2MConnHolderDecoder::restartZombieTmrExt(const bool &fastMode)
{
    myZombieKiller.isWaiting4answer = false;

    emit startTmrZombieKiller(fastMode ? 5000 : myZombieKiller.zombieMsec);
}

//---------------------------------------------------------------------------------------

void M2MConnHolderDecoder::fastZombieCheckSmart()
{
    if(myZombieKiller.isWaiting4answer)
        return;//the answer is waited
    //create a request and wait
    restartZombieTmrExt(true);


}

//---------------------------------------------------------------------------------------

void M2MConnHolderDecoder::onConnectionIsDown()
{
    if(!myStateParams.mMac.isEmpty())
        emit removeMyId2Hash(myStateParams.mMac, connId.otherId);
    myStateParams.mMac.clear();

}

//---------------------------------------------------------------------------------------

void M2MConnHolderDecoder::startStopSuicideTmr(QString remIpDescr, bool start)
{
    if(remIpDescr.isEmpty())
        return;
    if(remIpDescr != connId.otherId)
        return;

    if(start){
        if(!connId.stopAll)
            ask2closeTheConnection(tr("M2MServer wants me to die"));
    }else{
        emit onM2MServerAcceptedMe();//stop suicide timer
        QTimer::singleShot(11, this, SIGNAL(killTemporaryObjects()));//kill suicide timer
    }

}

//---------------------------------------------------------------------------------------

void M2MConnHolderDecoder::startSuicide()
{
    if(!connId.stopAll)
        ask2closeTheConnection(tr("suicide is true"));

}

//---------------------------------------------------------------------------------------
