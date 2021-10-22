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

#include "m2mserverfortwo.h"

///[!] svaha-service
#include "m2m-service-src/data-transmission/m2mdatasocket.h"



M2MServerForTwo::M2MServerForTwo(const qint32 &msec4zombie, const qint32 &msecAlive, const bool &verboseMode, QObject *parent) : QTcpServer(parent)
{
    myStateParams.verboseMode = verboseMode;
    myStateParams.msec4zombie = msec4zombie;
//    myStateParams.msecAlive = msecAlive;

    if(msecAlive > 0)
        QTimer::singleShot(msecAlive, this, SLOT(onZombie()));
}

quint16 M2MServerForTwo::findFreePort(const quint16 &minPort, const quint16 &maxPort)
{
    for(quint16 port = minPort; port < maxPort; port++){
        if(listen(QHostAddress::Any, port))
            return port;
    }
    return 0;
}

QTimer *M2MServerForTwo::createZombieTimer(const int &msec, const QString &tmrName)
{
    QTimer *zombieTmr = new QTimer(this);
    zombieTmr->setObjectName(tmrName);
    zombieTmr->setInterval(msec);// sLoader.loadOneSett(SETT_ZOMBIE_MSEC).toInt() );//15 * 60 * 1000);
    zombieTmr->setSingleShot(true);
    connect(zombieTmr, &QTimer::timeout, this, &M2MServerForTwo::onZombie);
    connect(this, &M2MServerForTwo::stopAllNow, zombieTmr, &QTimer::stop);
    zombieTmr->start();
    return zombieTmr;
}

void M2MServerForTwo::incomingConnection(qintptr handle)
{
    if(myStateParams.verboseMode)
        qDebug() << "inConn " << serverPort();

    M2MDataSocket *socket = new M2MDataSocket(this);
    connect(this, &M2MServerForTwo::stopAllNow, socket, &M2MDataSocket::onDisconn);

    if(!socket->setSocketDescriptor(handle) || myStateParams.connCounter > 1 ){ // the 3rd is not allowed

        if(myStateParams.verboseMode)
            qDebug() << "M2MServerForTwo incomingConnection if(!socket->setSocketDescriptor(socketDescr)){" <<
                        socket->errorString() << socket->socketDescriptor() <<
                        socket->localAddress() << socket->peerAddress() << myStateParams.connCounter;
        socket->close();
        socket->deleteLater();

        if(myStateParams.connCounter > 1)
            return;
        onZombie();
        return;
    }

    if(myStateParams.verboseMode)
        qDebug() << "M2MServerForTwo onNewConnection " << socket->peerAddress() << socket->peerName() << socket->peerPort();


    myStateParams.connCounter++;
    if(myStateParams.connCounter == 1){//the 1st
        connect(socket, &M2MDataSocket::mReadData, this, &M2MServerForTwo::data2Remote2);
        connect(this, &M2MServerForTwo::data2Remote1, socket, &M2MDataSocket::mWrite2socket);
        //enable buffering, until the second is not connected
        connect(this, &M2MServerForTwo::data2Remote2, this, &M2MServerForTwo::add2TmpBuff);

    }else{
        //disable buffering, because the second is connected
        disconnect(this, &M2MServerForTwo::data2Remote2, this, &M2MServerForTwo::add2TmpBuff);

        connect(socket, &M2MDataSocket::mReadData, this, &M2MServerForTwo::data2Remote1);
        connect(this, &M2MServerForTwo::data2Remote2, socket, &M2MDataSocket::mWrite2socket);

        if(!myStateParams.buffArr.isEmpty()){
            socket->mWrite2socket(myStateParams.buffArr);
//            emit data2Remote2(buffArr);
            myStateParams.buffArr.clear();
        }


    }
    connect(socket, &M2MDataSocket::iAmDisconn, this, &M2MServerForTwo::onOneDisconn);
    connect(socket, SIGNAL(iAmDisconn()), this, SLOT(onOneDisconn()) );


    const bool socketIsAlive = (socket->state() == QAbstractSocket::ConnectedState);
    if(!socketIsAlive)
        QTimer::singleShot(111, socket, SLOT(onDisconn()));// SLOT(onDisconn()) );

}

void M2MServerForTwo::onThreadStarted()
{
    auto *tmr1 = createZombieTimer(myStateParams.msec4zombie, "tmr1");
    auto *tmr2 = createZombieTimer(myStateParams.msec4zombie, "tmr2");

    connect(this, SIGNAL(data2Remote1(QByteArray)), tmr1, SLOT(start()));
    connect(this, SIGNAL(data2Remote2(QByteArray)), tmr2, SLOT(start()));

}

void M2MServerForTwo::onOneDisconn()
{
    if(myStateParams.connCounter < 0)
        return;

    if(myStateParams.verboseMode)
        qDebug() << "M2MServerForTwo onOneDisconn ";

    myStateParams.connCounter = -1;
    onZombie();
}

void M2MServerForTwo::onZombie()
{
    if(myStateParams.verboseMode)
        qDebug() << "M2MServerForTwo onZombie " ;
    myStateParams.connCounter = -1;
    close();
    emit stopAllNow();

    deleteLater();
}

void M2MServerForTwo::add2TmpBuff(QByteArray buffArr)
{
    if(myStateParams.connCounter != 1)
        return;

    myStateParams.buffArr.append(buffArr);
    if(myStateParams.buffArr.length() > 50000)
        onZombie();
//        this->buffArr = this->buffArr.mid(this->buffArr.length() - 50000);
    if(myStateParams.verboseMode)
        qDebug() << "bufferization enabled ";
}
