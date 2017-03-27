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
#include "cerber4matilda.h"
#include "cerber4matildasocket.h"
#include <QtCore>

//----------------------------------------------------------------------------------------------------------------------------
Cerber4matilda::Cerber4matilda( QObject *parent) : QTcpServer(parent)
{

}
//----------------------------------------------------------------------------------------------------------------------------
bool Cerber4matilda::startServer(quint16 port)
{
    return listen(QHostAddress::Any, port);
}

//----------------------------------------------------------------------------------------------------------------------------
void Cerber4matilda::incomingConnection(qintptr handle)
{
    Cerber4matildaSocket *socket = new Cerber4matildaSocket;
    if(!socket->setSocketDescriptor(handle)){
        qDebug() << "incomingConnection if(!socket->setSocketDescriptor(socketDescr)){";
        delete socket;
        return;
    }
    qDebug() << "onNewConnection cerver " << socket->peerAddress() << socket->peerName() << socket->peerPort();
    QThread *thread = new QThread(this);
    socket->moveToThread(thread);

    connect(socket, SIGNAL(destroyed(QObject*)), thread, SLOT(quit()) );
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()) );
    connect(thread, SIGNAL(started()), socket, SLOT(onThrdStarted()) );


    connect(socket, SIGNAL(getHashRemoteIdAndDevId(QString, bool)), this, SIGNAL(getHashRemoteIdAndDevId(QString, bool)) );
    connect(socket, SIGNAL(removeCerverID(QString)), this, SIGNAL(removeCerverID(QString)) );
    connect(socket, SIGNAL(killClientNow(QString,bool)), this, SIGNAL(killClientNow(QString,bool)) );
    connect(this, SIGNAL(remoteIdAndDevId(QStringHash,QStringHash,QStringHash,QString)), socket, SLOT(remoteIdAndDevId(QStringHash,QStringHash,QStringHash,QString)) );


    QVariantHash hash;
    hash.insert("QDS", QDataStream::Qt_5_6);
    qsrand(qrand());
    hash.insert("RND", qrand() % ((0xFFFFFFF + 1) - 1) + 1);
    hash.insert("name", QString("Matilda"));


    hash.insert("UTC", QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd hh:mm:ss"));
    hash.insert("UOFT", QDateTime::currentDateTime().utcOffset());




    socket->writeTmpStamp(hash);

    thread->start();
}
//----------------------------------------------------------------------------------------------------------------------------

