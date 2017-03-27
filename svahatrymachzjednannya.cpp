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
#include "svahatrymachzjednannya.h"

#include <QThread>

#include "svahadladvoh.h"

#include "socketdlyatrymacha.h"


//----------------------------------------------------------------------------------------------------------------------------
SvahaTrymachZjednannya::SvahaTrymachZjednannya(QObject *parent) : QTcpServer(parent)
{
    svahaServicePort = 0;
    SettLoader4svaha sLoader;
//    QFile fileConf(QString("%1/svaha.conf").arg(qApp->applicationDirPath()));
//    if(fileConf.open(QFile::ReadOnly)){
//        QStringList list = QString(fileConf.readAll()).split("\n", QString::SkipEmptyParts) ;
//        qDebug() << list;
//        fileConf.close();
//        for(int i = 0, iMax = list.size(); i < iMax; i++){
//            qDebug() << "list " << i << list.at(i);
//            if(list.at(i).left(15) == "matilda-dev-ip="){
//                server4matildadev = list.at(i).mid(15).simplified().remove(" ");
//            }else{
//                if(list.at(i).left(16) == "matilda-conf-ip="){
//                    server4matildaConf = list.at(i).mid(16).simplified().remove(" ");
//                }else{
//                    if(list.at(i).left(19) == "svaha-service-port="){
//                        svahaServicePort = list.at(i).mid(19).simplified().remove(" ").toUInt();
//                    }
//                }
//            }
//        }
//    }else{
//        qDebug() << "file "  << fileConf.fileName() << fileConf.errorString();
//    }
    sLoader.checkDefSett();
    server4matildaConf = sLoader.loadOneSett(SETT_MATILDA_CONF_IP).toString();
    server4matildadev = sLoader.loadOneSett(SETT_MATILDA_DEV_IP).toString();
    svahaServicePort = (quint16)sLoader.loadOneSett(SETT_SVAHA_SERVICE_PORT).toUInt();

    if(server4matildadev.isEmpty())
        server4matildadev = "svaha.ddns.net";

    if(server4matildaConf.isEmpty())
        server4matildaConf = "svaha.ddns.net";

    if(svahaServicePort < 1000 || svahaServicePort >= 65535)
        svahaServicePort = 65000;




    qDebug() << "SvahaTrymachZjednannya=" << server4matildaConf << server4matildadev << svahaServicePort;

//    for(int i = 0; i < 11; i++)
//        addMyId2Hash(QString("someId_%1").arg(i), QString("macaddr_%1").arg(i).split(","), QString("remote_id_%1").arg(i));
}
//----------------------------------------------------------------------------------------------------------------------------
bool SvahaTrymachZjednannya::startService()
{
    if(listen(QHostAddress::Any, svahaServicePort)){

        Cerber4matilda *server = new Cerber4matilda;
        SettLoader4svaha sLoader;

        if(!server->startServer(sLoader.loadOneSett(SETT_CERBERUS_PORT).toUInt())){
            qDebug() << "startService 0 " << sLoader.loadOneSett(SETT_CERBERUS_PORT).toUInt() << server->errorString();
            return false;
        }

        qRegisterMetaType<QStringHash>("QStringHash");

        QThread *thread = new QThread(this);
        server->moveToThread(thread);

        connect(server, SIGNAL(getHashRemoteIdAndDevId(QString, bool)), this, SLOT(getHashRemoteIdAndDevId(QString, bool)) );
        connect(server, SIGNAL(removeCerverID(QString)), this, SLOT(removeCerverID(QString)) );
        connect(server, SIGNAL(killClientNow(QString,bool)), this, SIGNAL(killClientNow(QString,bool)) );
        connect(this, SIGNAL(remoteIdAndDevId(QStringHash,QStringHash,QStringHash,QString)), server, SIGNAL(remoteIdAndDevId(QStringHash,QStringHash,QStringHash,QString)) );

        connect(server, SIGNAL(destroyed(QObject*)), thread, SLOT(quit()) );
        connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()) );

        thread->start();

        tmrCerver.setInterval(100);
        tmrCerver.setSingleShot(true);

        connect(this, SIGNAL(updateCerver()), &tmrCerver, SLOT(start()) );
        connect(&tmrCerver, SIGNAL(timeout()), this, SLOT(sendCerverInfo()) );
        return true;
    }
    qDebug() << "startService 10 " << svahaServicePort << errorString();
    return false;
}
//----------------------------------------------------------------------------------------------------------------------------
void SvahaTrymachZjednannya::addMyId2Hash(QString id, QStringList macList, QString remoteId)
{
    QString dt = QDateTime::currentDateTimeUtc().toString("yyyyMMddhhmmss");
    for(int i = 0, iMax = macList.size(); i < iMax; i++){
        hashMacDevId.insert(macList.at(i), id);
        hashMacRemoteId.insert(macList.at(i), remoteId);
        hashMacAddTime.insert(macList.at(i), dt);
        qDebug() << "addMyId2Hash" << i << macList.at(i) << id << remoteId;
    }   
    emit updateCerver();

}
//----------------------------------------------------------------------------------------------------------------------------
void SvahaTrymachZjednannya::removeMyId2Hash(QStringList macList)
{
    for(int i = 0, iMax = macList.size(); i < iMax; i++){

        qDebug() << "removeMyId2Hash" << macList.at(i) << hashMacDevId.value(macList.at(i)) << hashMacRemoteId.value(macList.at(i)) << hashMacAddTime.value(macList.at(i));

        hashMacDevId.remove(macList.at(i));
        hashMacRemoteId.remove(macList.at(i));
        hashMacAddTime.remove(macList.at(i));

        emit checkThisMac(macList.at(i));

    }
    emit updateCerver();

}
//----------------------------------------------------------------------------------------------------------------------------
void SvahaTrymachZjednannya::connMe2ThisIdOrMac(QString macOrId, bool isMac, QString myRemoteId, QString rIp)
{
    QStringList list;
    QString mac, id, remSid;
    if(isMac){
        macOrId = macOrId.toUpper();

        if(hashMacDevId.contains(macOrId) && !hashMacDevId.value(macOrId).isEmpty()){
            list.append(QString("%1@%2").arg(macOrId).arg(hashMacDevId.value(macOrId)));
            mac = macOrId;
            id = hashMacDevId.value(macOrId);
            remSid = hashMacRemoteId.value(macOrId);
        }

    }else{
        qDebug() << "useMac = false" << macOrId << hashMacDevId.values().contains(macOrId);
        if(hashMacDevId.values().contains(macOrId)){
             QList<QString> lKeys = hashMacDevId.keys();
             id = macOrId;
             remSid.clear();
             QStringList remIdList;
             for(int i = 0, iMax = lKeys.size(); i < iMax; i++){
                 qDebug() << i << lKeys.at(i) << hashMacDevId.value(lKeys.at(i)) << macOrId;
                 if(hashMacDevId.value(lKeys.at(i)) == macOrId){
                     list.append(QString("%1@%2").arg(lKeys.at(i)).arg(macOrId));
                     mac = lKeys.at(i);
                     remSid = hashMacRemoteId.value(lKeys.at(i));
                     remIdList.append(remSid);
                     qDebug() << "add2list" << list.last();
                 }
             }
             if(remIdList.size() > 1){// якщо на одному пристрої більше одного мак адресу, то беру останній мак адрес
                 remIdList.removeDuplicates();
                 if(remIdList.size() == 1){
                     list.clear();
                     list.append(QString("%1@%2").arg(mac).arg(macOrId));
                 }
             }
        }
    }
    qDebug() << "connMe2this" << list << macOrId << isMac << myRemoteId << id << mac << remSid;
    if(list.size() == 1){//знайдено одного унікального, виконую з’єднання з ним

        SvahaDlaDvoh *server = new SvahaDlaDvoh;
        SettLoader4svaha sLoader;
        quint16 startPort = sLoader.loadOneSett(SETT_SVAHA_DATA_START_PORT).toUInt();
        quint16 endPort = startPort + sLoader.loadOneSett(SETT_SVAHA_DATA_PORT_COUNT).toUInt();

        if(startPort > 65534 || endPort > 65534 || startPort < 1000 || endPort < 1000){
            startPort = SettLoader4svaha::defSETT_SVAHA_DATA_START_PORT();
            endPort = startPort + SettLoader4svaha::defSETT_SVAHA_DATA_PORT_COUNT();
        }


        quint16 svahaPort = server->findFreePort(startPort, endPort);
        qDebug() << "start SvahaDlaDvoh" << svahaPort;
        if(svahaPort == 0){
            emit onResourBusy(myRemoteId);
            server->deleteLater();
        }else{
            QThread *thread = new QThread(this);
            server->moveToThread(thread);
            connect(thread, SIGNAL(started()), server, SLOT(onThrdStarted()) );
            connect(server, SIGNAL(destroyed(QObject*)), thread, SLOT(quit()));
            connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()) );

            emit startConn(server4matildaConf, svahaPort, myRemoteId);
            emit startConn(server4matildadev, svahaPort, id, mac, remSid , rIp);
            thread->start();
        }

    }else{// не знайдено жодного або знайдено к-ка однакових, передається список для уточнення
        emit connMe2ThisIdOrMacSig(list, myRemoteId);
    }
}
//----------------------------------------------------------------------------------------------------------------------------
void SvahaTrymachZjednannya::getHashRemoteIdAndDevId(QString id, bool add2auto)
{


    if(add2auto && !cerverIdList.contains(id))
        cerverIdList.append(id);
    else if(!add2auto && cerverIdList.contains(id))
        cerverIdList.removeOne(id);
    emit remoteIdAndDevId(hashMacRemoteId, hashMacDevId, hashMacAddTime, id);
}
//----------------------------------------------------------------------------------------------------------------------------
void SvahaTrymachZjednannya::removeCerverID(QString id)
{
    cerverIdList.removeOne(id);
}

//----------------------------------------------------------------------------------------------------------------------------
void SvahaTrymachZjednannya::incomingConnection(qintptr handle)
{
    SocketDlyaTrymacha *socket = new SocketDlyaTrymacha;
    if(!socket->setSocketDescriptor(handle)){
        qDebug() << "incomingConnection if(!socket->setSocketDescriptor(socketDescr)){";
        socket->deleteLater();
        return;
    }
    qDebug() << "SvahaTrymachZjednannya onNewConnection " << socket->peerAddress() << socket->peerName() << socket->peerPort();


    QThread *thread = new QThread(this);
    socket->moveToThread(thread);

    connect(socket, SIGNAL(destroyed(QObject*)), thread, SLOT(quit()) );
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()) );
    connect(thread, SIGNAL(started()), socket, SLOT(onThrdStarted()) );

    connect(socket, SIGNAL(connMe2ThisIdOrMac(QString,bool,QString, QString)), this, SLOT(connMe2ThisIdOrMac(QString,bool,QString,QString)) );
    connect(socket, SIGNAL(addMyId2Hash(QString,QStringList,QString)), this, SLOT(addMyId2Hash(QString,QStringList,QString)) );
    connect(socket, SIGNAL(removeMyId2Hash(QStringList)), this, SLOT(removeMyId2Hash(QStringList)) );

    connect(this, SIGNAL(connMe2ThisIdOrMacSig(QStringList,QString)), socket, SLOT(connMe2ThisIdOrMacSlot(QStringList,QString)) );
    connect(this, SIGNAL(startConn(QString,int,QString)), socket, SLOT(startConn(QString,int,QString)) );
    connect(this, SIGNAL(startConn(QString,int,QString,QString,QString,QString)), socket, SLOT(startConn(QString,int,QString,QString,QString,QString)) );
    connect(this, SIGNAL(onResourBusy(QString)), socket, SLOT(onResourBusy(QString)) );
    connect(this, SIGNAL(checkThisMac(QString)), socket, SLOT(checkThisMac(QString)) );
    connect(this, SIGNAL(killClientNow(QString,bool)), socket, SLOT(killClientNow(QString,bool)) );


    thread->start();

}
//----------------------------------------------------------------------------------------------------------------------------
void SvahaTrymachZjednannya::sendCerverInfo()
{
    for(int i = 0, iMax = cerverIdList.size(); i < iMax; i++)
        emit remoteIdAndDevId(hashMacRemoteId, hashMacDevId, hashMacAddTime, cerverIdList.at(i));

}
//----------------------------------------------------------------------------------------------------------------------------
