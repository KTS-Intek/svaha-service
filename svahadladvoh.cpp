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
#include "svahadladvoh.h"
#include <QTimer>

#include "socketprosto.h"
#include "settloader4svaha.h"

SvahaDlaDvoh::SvahaDlaDvoh(QObject *parent) : QTcpServer(parent)
{
    connCounter = 0;
    setMaxPendingConnections(2);
}

quint16 SvahaDlaDvoh::findFreePort(quint16 minP, const quint16 &maxP)
{
    connCounter = 0;
    for( ; minP < maxP; minP++){
        if(listen(QHostAddress::Any, minP)){

            return minP;
        }
    }

    return 0;

}

void SvahaDlaDvoh::incomingConnection(qintptr handle)
{
    qDebug() << "inConn " << serverPort();
    SocketProsto *socket = new SocketProsto;
    if(!socket->setSocketDescriptor(handle) || connCounter > 1 ){
        socket->close();
        qDebug() << "SvahaDlaDvoh incomingConnection if(!socket->setSocketDescriptor(socketDescr)){";
        socket->deleteLater();
        return;
    }
    qDebug() << "SvahaDlaDvoh onNewConnection " << socket->peerAddress() << socket->peerName() << socket->peerPort();

    connCounter++;
    if(connCounter == 1){//перший
        connect(socket, SIGNAL(mReadData(QByteArray)), this, SIGNAL(data2Remote2(QByteArray)) );
        connect(this, SIGNAL(data2Remote(QByteArray)), socket, SLOT(mWrite2socket(QByteArray)));
        connect(this, SIGNAL(data2Remote2(QByteArray)), this, SLOT(add2TmpBuff(QByteArray)) );
    }else{
        disconnect(this, SIGNAL(data2Remote2(QByteArray)), this, SLOT(add2TmpBuff(QByteArray)) );

        connect(socket, SIGNAL(mReadData(QByteArray)), this, SIGNAL(data2Remote(QByteArray)) );
        connect(this, SIGNAL(data2Remote2(QByteArray)), socket, SLOT(mWrite2socket(QByteArray)));

        if(!buffArr.isEmpty()){
            socket->mWrite2socket(buffArr);
//            emit data2Remote2(buffArr);
            buffArr.clear();
        }


    }

    connect(socket, SIGNAL(iAmDisconn()), this, SLOT(onOneDisconn()) );
    connect(this, SIGNAL(stopAllNow()), socket, SLOT(onDisconn()) );


}

void SvahaDlaDvoh::onThrdStarted()
{
    SettLoader4svaha sLoader;
    QTimer *zombieTmr = new QTimer(this);
    zombieTmr->setInterval(sLoader.loadOneSett(SETT_ZOMBIE_MSEC).toInt() );//15 * 60 * 1000);
    zombieTmr->setSingleShot(true);
    connect(this, SIGNAL(data2Remote(QByteArray)), zombieTmr, SLOT(start()) );
    connect(zombieTmr, SIGNAL(timeout()), this, SLOT(onZombie()) );
    connect(this, SIGNAL(stopAllNow()), zombieTmr, SLOT(stop()) );
    zombieTmr->start();

    QTimer *zombieTmr2 = new QTimer(this);
    zombieTmr->setInterval(sLoader.loadOneSett(SETT_ZOMBIE_MSEC).toInt() );//15 * 60 * 1000);
    zombieTmr->setSingleShot(true);
    connect(this, SIGNAL(data2Remote2(QByteArray)), zombieTmr2, SLOT(start()) );
    connect(zombieTmr2, SIGNAL(timeout()), this, SLOT(onZombie()) );
    connect(this, SIGNAL(stopAllNow()), zombieTmr2, SLOT(stop()) );
    zombieTmr2->start();
}

void SvahaDlaDvoh::onOneDisconn()
{
    if(connCounter < 0)
        return;

    qDebug() << "SvahaDlaDvoh onOneDisconn ";

    connCounter = -1;
    onZombie();
}

void SvahaDlaDvoh::onZombie()
{
    qDebug() << "SvahaDlaDvoh onZombie ";
    connCounter = -1;
    close();
    emit stopAllNow();

    deleteLater();
}

void SvahaDlaDvoh::add2TmpBuff(QByteArray buffArr)
{
    if(connCounter != 1)
        return;

    this->buffArr.append(buffArr);
    if(this->buffArr.length() > 50000)
        this->buffArr = this->buffArr.mid(this->buffArr.length() - 50000);
    qDebug() << "bufferization enabled ";

}
