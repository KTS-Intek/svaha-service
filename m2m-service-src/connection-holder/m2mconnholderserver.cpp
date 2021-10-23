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
#include "m2mconnholderserver.h"


///[!] svaha-service
#include "m2m-service-src/data-transmission/m2mserverfortwo.h"
#include "m2m-service-src/connection-holder/m2mconnholdersocket.h"

//----------------------------------------------------------------------------------------------------

M2MConnHolderServer::M2MConnHolderServer(const bool &verboseMode, QObject *parent)
    : M2MConnHolderServerBase(verboseMode, parent)
{

}

//----------------------------------------------------------------------------------------------------

void M2MConnHolderServer::onThreadStarted()
{

    accesManager = new IPAccessManager(this);
    connect(accesManager, &IPAccessManager::add2systemLog, this, &M2MConnHolderServer::addEvent2log);
    connect(this, &M2MConnHolderServer::setAllowAndBlockList, accesManager, &IPAccessManager::setAllowAndBlockList);
    connect(this, &M2MConnHolderServer::blockThisIP, accesManager, &IPAccessManager::blockThisIP);

    emit gimmeSettings();

}

//----------------------------------------------------------------------------------------------------

void M2MConnHolderServer::try2startServer()
{
    if(startService()){
        return;
    }
    QTimer::singleShot(999, this, SLOT(try2startServer()));
}

//----------------------------------------------------------------------------------------------------

void M2MConnHolderServer::setServicePortSmart(quint16 port)
{
    if(port != myParams.port){
        emit addEvent2log(QString("New port is %1. Restarting...").arg(int(port)));
        close();
        myParams.port = port;

    }
    if(!isListening())
        QTimer::singleShot(999, this, SLOT(try2startServer()));

}


//----------------------------------------------------------------------------------------------------

void M2MConnHolderServer::connMe2ThisIdOrMac(QString macOrId, bool isMac, QString myRemoteId, QString rIp)
{
    const QStringList connectL = isMac ? getDevicesWithThisMAC(macOrId.toUpper()) : getDevicesWithThisID(macOrId);
    if(connectL.size() != 1){
        // не знайдено жодного або знайдено к-ка однакових, передається список для уточнення
        emit onFailed2connect2oneDev(connectL, myRemoteId);
        return;
    }

    //start connection

    M2MServerForTwo *server = new M2MServerForTwo(
                myParams.socketTimeouts.zombieMsec, //between message
                myParams.socketTimeouts.msecAlive,  //time to live,
                myParams.verboseMode);      //qdebug messages

    const quint16 server4duoPort = server->findFreePort(myParams.serverDataStart, myParams.serverDataEnd);

    if(server4duoPort == 0){
        emit onResourBusy(myRemoteId);
        server->deleteLater();
        return;
    }


    QThread *thread = new QThread(this);
    thread->setObjectName(QString("%1\n%2").arg(macOrId).arg(myRemoteId));
    server->moveToThread(thread);
    connect(thread, SIGNAL(started()), server, SLOT(onThreadStarted()) );
    connect(server, SIGNAL(destroyed(QObject*)), thread, SLOT(quit()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()) );


    //I don't know why did I use differnt server addresses,
//    server4matildaConf = sLoader.loadOneSett(SETT_MATILDA_CONF_IP).toString();
//    server4matildadev = sLoader.loadOneSett(SETT_MATILDA_DEV_IP).toString();
//    emit startConn(server4matildaConf, server4duoPort, myRemoteId);
//    emit startConn(server4matildadev, server4duoPort, id, mac, remSid , rIp);

    //let UCon connect first, to avoid data bufferezation
    emit startConn4UCon(server4duoPort, myRemoteId);

    //    void startConnMatildaDev(QString serverIp, int serverPort, QString objId, QString objMac, QString objSocketId, QString rIp);//server: ip, port, socket id mac <remote ip>:<descr>

    const QString macUpper = MatildaProtocolHelperV2::getMacFromM2MMacIDLine(connectL.first());// isMac ? macOrId.toUpper() : ;


//    QString mac, id, remSid;
    const QString id = myTable.hashMacDevId.value(macUpper);
    const QString remSid = myTable.hashMacRemoteId.value(macUpper);

    emit startConnMatildaDev(server4duoPort, id, macUpper, remSid, rIp);

    thread->start();

}

//----------------------------------------------------------------------------------------------------

void M2MConnHolderServer::onThisDecoderReady(M2MConnHolderDecoder *decoder)
{
    if(decoder){
        connect(decoder, &M2MConnHolderDecoder::connMe2ThisIdOrMac  , this, &M2MConnHolderServer::connMe2ThisIdOrMac);
        connect(this, &M2MConnHolderServer::killClientNow               , decoder, &M2MConnHolderDecoder::killClientNow);

        connect(decoder, &M2MConnHolderDecoder::removeThisIpFromTemporaryBlockList, accesManager, &IPAccessManager::removeThisIpFromTemporaryBlockList);

        onThisDecoderReadyBase(decoder);
    }
}


//----------------------------------------------------------------------------------------------------


void M2MConnHolderServer::incomingConnection(qintptr handle)
{
    M2MConnHolderSocket *socket = new M2MConnHolderSocket;
    if(!socket->setSocketDescriptor(handle)){
        if(myParams.verboseMode)
            qDebug() << "incomingConnection if(!socket->setSocketDescriptor(socketDescr)){";
        socket->deleteLater();
        return;
    }

    const QString strIP = NetworkConvertHelper::showNormalIP(socket->peerAddress());// lsender.at(i));


    if(accesManager->isIPblockedByTheAllowList(strIP) ||
            accesManager->isIPblockedByTheBlockList(strIP) ||
            accesManager->isIPblockedByTheTemporaryBlockList(strIP)){
        if(myParams.verboseMode)
            qDebug() << "incomingConnection ignore this ip " << strIP;

        socket->onDisconn();
        emit addEvent2log(QString("M2MConnHolderServer in.c. %1, sd:%2, Access denied!").arg(strIP).arg(handle));
        return;
    }
    accesManager->addThisIPToTempraryBlockListQuiet(strIP);

    QThread *thread = new QThread(this);
    thread->setObjectName(QString("%1/%2").arg(strIP).arg(QString::number(socketDescriptor())));
    socket->moveToThread(thread);
//    socket->createDecoder(myParams.verboseMode);//is it right???

    connect(socket, SIGNAL(destroyed(QObject*)), thread, SLOT(quit()) );
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()) );


    if(myParams.verboseMode)
        connect(thread, SIGNAL(started()), socket, SLOT(onThreadStartedVerb()) );
    else
        connect(thread, SIGNAL(started()), socket, SLOT(onThreadStarted()) );


    connect(socket, &M2MConnHolderSocket::onThisDecoderReady, this, &M2MConnHolderServer::onThisDecoderReady);
    connect(this, &M2MConnHolderServer::onTimeoutsChanged               , socket, &M2MConnHolderSocket::onTimeoutsChanged);

    connect(this, &M2MConnHolderServer::killAllClinets, socket, &M2MConnHolderSocket::onDisconn);

    socket->onTimeoutsChanged(myParams.socketTimeouts);


    thread->start();


//    socket->set









}

//----------------------------------------------------------------------------------------------------
