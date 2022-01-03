/****************************************************************************
**
**   Copyright © 2016-2021 The KTS-INTEK Ltd.
**   Contact: https://www.kts-intek.com
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
#include "m2mconnholderserverbase.h"


//-----------------------------------------------------------------------------------------

M2MConnHolderServerBase::M2MConnHolderServerBase(const bool &verboseMode, QObject *parent) : QTcpServer(parent)
{
    myParams.verboseMode = verboseMode;
    myParams.port = 0;
    makeCustomTypeRegistration();

}

//-----------------------------------------------------------------------------------------

void M2MConnHolderServerBase::makeCustomTypeRegistration()
{
    qRegisterMetaType<QStringHash>("QStringHash");
    qRegisterMetaType<QStringHashHash>("QStringHashHash");
    qRegisterMetaType<QList<QDateTime> >("QList<QDateTime>");
    qRegisterMetaType<ConnectionTimeouts>("ConnectionTimeouts");


}


//-----------------------------------------------------------------------------------------


bool M2MConnHolderServerBase::startService()
{
    if(myParams.port == 0)
        return true;

    if(listen(QHostAddress::Any, myParams.port)){
        emit addEvent2log(QString("Connection holder is listenning on %1").arg(myParams.port));
        return true;
    }

    emit onFailed2startServer(QString("Failed to start connection holder server. %1").arg(errorString()));
    if(myParams.verboseMode)
        qDebug() << "startService 10 " << myParams.port << errorString();
    return false;
}

//-----------------------------------------------------------------------------------------

QStringList M2MConnHolderServerBase::getDevicesWithThisID(const QString &id)
{
    QStringList out;

    if(id.isEmpty())
        return out;//no such device



    auto macl = myTable.hashMacDevId.keys(id);

    if(macl.isEmpty())
        return out;

    std::sort(macl.begin(), macl.end());
    //one device can have more than one MAC, so it is necessary to check socket IDs

//    myTable.hashMacDevId.insert(mac, objId);
//    myTable.hashMacRemoteId.insert(mac, remIpDescr);
//    myTable.hashMacAddTime.insert(mac, dt);
//    myTable.hashMac2objectIfo.insert(mac, hashObjIfo);

    QStringList usedRemIpDescr;
    for(int i = 0, imax = macl.size(); i < imax; i++){
        const QString macUpper = macl.at(i);
        if(macUpper.isEmpty())
            continue;
        const QString remIpDescr = myTable.hashMacRemoteId.value(macUpper);
        if(usedRemIpDescr.contains(remIpDescr))
            continue;//it has such socket, ignore

        usedRemIpDescr.append(remIpDescr);
        out.append(MatildaProtocolHelperV2::getM2MMacIDLine(macUpper, id));
    }
    return out;
}

//-----------------------------------------------------------------------------------------

QStringList M2MConnHolderServerBase::getDevicesWithThisMAC(const QString &macUpper)
{
    QStringList out;
    if(!myTable.hashMacDevId.contains(macUpper))
        return out;//no such device

//    const QString devid = myTable.hashMacDevId.value(mac);

    out.append(MatildaProtocolHelperV2::getM2MMacIDLine(macUpper, myTable.hashMacDevId.value(macUpper)));
    return out;

}

void M2MConnHolderServerBase::stopAllSlot()
{
    if(isListening())
        emit addEvent2log(QString("Closing..."));
    close();

//    emit killAllClinets();

}


//-----------------------------------------------------------------------------------------

void M2MConnHolderServerBase::setTimeouts(int zombieMsec, int msecAlive, int timeOutGMsec, int timeOutBMsec)
{
    if(zombieMsec > 0)
    myParams.socketTimeouts.zombieMsec = zombieMsec;
    if(msecAlive > 0)
    myParams.socketTimeouts.msecAlive = msecAlive;

    if(timeOutGMsec > 0)
    myParams.socketTimeouts.timeOutGMsec = timeOutGMsec;
    if(timeOutBMsec > 0)
    myParams.socketTimeouts.timeOutBMsec = timeOutBMsec;
    emit onTimeoutsChanged(myParams.socketTimeouts);

}

//-----------------------------------------------------------------------------------------

void M2MConnHolderServerBase::setBackupWorkDirectory(QString workDir)
{
    myParams.workDir = workDir;
    emit onBackupWorkDirectoryChanged(myParams.workDir);
}

//-----------------------------------------------------------------------------------------

void M2MConnHolderServerBase::setDataConnectionParams(QString serverDataIP, quint16 serverDataStart, quint16 serverDataEnd)
{
    myParams.serverDataIP = serverDataIP;
    myParams.serverDataStart = serverDataStart;
    myParams.serverDataEnd = serverDataEnd;
    emit onDataConnectionParamsChanged(myParams.serverDataIP, myParams.serverDataStart, myParams.serverDataEnd);
}


//-----------------------------------------------------------------------------------------

void M2MConnHolderServerBase::addMyId2Hash(QString objId, QStringList macl, QString remIpDescr, QStringHash hashObjIfo, bool add2sync)
{
    if(remIpDescr.isEmpty())
        return;

    emit addEvent2log(QString("addMyId2Hash %1, %2, %3").arg(remIpDescr, objId, macl.join(" ")));

    const QString dt = QDateTime::currentDateTimeUtc().toString("yyyyMMddhhmmss");
    for(int i = 0, iMax = macl.size(); i < iMax; i++){
        const QString mac = macl.at(i);

        myTable.hashMacDevId.insert(mac, objId);
        myTable.hashMacRemoteId.insert(mac, remIpDescr);
        myTable.hashMacAddTime.insert(mac, dt);
        myTable.hashMac2objectIfo.insert(mac, hashObjIfo);

        if(myParams.verboseMode){
            if( i == 0 )
                qDebug() << "hashObjIfo " << objId << remIpDescr << hashObjIfo;
            qDebug() << "addMyId2Hash" << i << objId << mac ;
        }
    }
    if(add2sync)
        emit onConnectedTheseMacs(macl);

    if(!remIpDescr.isEmpty())
        emit startStopSuicideTmr(remIpDescr, false);//stop suicide tmr

    if(!macl.isEmpty())
        tellThatConnectionTableChanged();

}

//-----------------------------------------------------------------------------------------

void M2MConnHolderServerBase::removeMyId2Hash(QStringList macl, QString remIpDescr)
{
    if(!remIpDescr.isEmpty())
        emit startStopSuicideTmr(remIpDescr, true);//start suicide tmr

    emit addEvent2log(QString("removeMyId2Hash %1, %2").arg(remIpDescr, macl.join(" ")));

    const int iMax = macl.size();
    for(int i = 0; i < iMax; i++){

        const QString mac = macl.at(i);
        if(myParams.verboseMode)
            qDebug() << "removeMyId2Hash" << mac << myTable.hashMacDevId.value(mac) << myTable.hashMacRemoteId.value(mac) << myTable.hashMacAddTime.value(mac);

        myTable.hashMacDevId.remove(mac);
        myTable.hashMacRemoteId.remove(mac);
        myTable.hashMacAddTime.remove(mac);
        myTable.hashMac2objectIfo.remove(mac);
/*
 * допускою ситуацію що є оданкові мак адреси (наприклад USB WiFi)
 * тому коли виконується відєднання, то перевіряю чи не залишилось когось з такою мак адресою
*/
        emit checkThisMac(mac);


    }
    emit onDisconnectedTheseMacs(macl, iMax);

    tellThatConnectionTableChanged();

}

void M2MConnHolderServerBase::setInfoAboutObj(QStringList macL, QStringHash objIfo, int counter)
{
    //    void infoAboutObj(QStringList macL, QStringHash objIfo, int counter);


    if(objIfo.isEmpty())
        return;//nothing to add

    for(int i = 0; i < counter; i++){
        const QString mac = macL.at(i);

        if(!myTable.hashMac2objectIfo.contains(mac))
            continue;//no such mac

        myTable.hashMac2objectIfo.insert(mac, objIfo);
    }
}

void M2MConnHolderServerBase::tellThatConnectionTableChanged()
{
    emit onConnectionTableChanged();
}

//-----------------------------------------------------------------------------------------

void M2MConnHolderServerBase::sendConnectionTable()
{
    emit onConnectionTableData(myTable.hashMacRemoteId, myTable.hashMacDevId, myTable.hashMacAddTime, myTable.hashMac2objectIfo);

}

void M2MConnHolderServerBase::onThisDecoderReadyBase(M2MConnHolderDecoder *decoder)
{
    if(decoder){
        connect(decoder, &M2MConnHolderDecoder::addMyId2Hash        , this, &M2MConnHolderServerBase::addMyId2Hash);
        connect(decoder, &M2MConnHolderDecoder::removeMyId2Hash     , this, &M2MConnHolderServerBase::removeMyId2Hash);
        connect(decoder, &M2MConnHolderDecoder::setInfoAboutObj     , this, &M2MConnHolderServerBase::setInfoAboutObj);

        connect(this, &M2MConnHolderServerBase::onFailed2connect2oneDev     , decoder, &M2MConnHolderDecoder::onFailed2connect2oneDev);
        //former  connMe2ThisIdOrMacSig
        connect(this, &M2MConnHolderServerBase::startConnMatildaDev         , decoder, &M2MConnHolderDecoder::startConnMatildaDev);
        connect(this, &M2MConnHolderServerBase::startConn4UCon              , decoder, &M2MConnHolderDecoder::startConn4UCon);
        connect(this, &M2MConnHolderServerBase::onResourBusy                , decoder, &M2MConnHolderDecoder::onResourBusy);
        connect(this, &M2MConnHolderServerBase::checkThisMac                , decoder, &M2MConnHolderDecoder::checkThisMac);



        connect(decoder, &M2MConnHolderDecoder::onSyncFileDownloaded, this, &M2MConnHolderServerBase::onSyncFileDownloaded);
        connect(decoder, &M2MConnHolderDecoder::onSyncRequestRemoteSha1isEqual, this, &M2MConnHolderServerBase::onSyncRequestRemoteSha1isEqual);

        connect(this, &M2MConnHolderServerBase::checkBackup4thisMac             , decoder, &M2MConnHolderDecoder::checkBackup4thisMac);

        connect(this, &M2MConnHolderServerBase::onDataConnectionParamsChanged   , decoder, &M2MConnHolderDecoder::onDataConnectionParamsChanged);
        connect(this, &M2MConnHolderServerBase::onBackupWorkDirectoryChanged    , decoder, &M2MConnHolderDecoder::onBackupWorkDirectoryChanged);

        connect(decoder, &M2MConnHolderDecoder::addError2Log, this, &M2MConnHolderServerBase::addEvent2log);

        connect(this, &M2MConnHolderServerBase::startStopSuicideTmr, decoder, &M2MConnHolderDecoder::startStopSuicideTmr);


        decoder->onDataConnectionParamsChanged(myParams.serverDataIP, myParams.serverDataStart, myParams.serverDataEnd);
        decoder->onBackupWorkDirectoryChanged(myParams.workDir);
        decoder->onEverythingIsConnected();//is it right??

    }

}


//-----------------------------------------------------------------------------------------
