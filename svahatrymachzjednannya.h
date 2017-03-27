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
#ifndef SVAHATRYMACHZJEDNANNYA_H
#define SVAHATRYMACHZJEDNANNYA_H

#include <QTcpServer>

#include <QObject>
#include <QtCore>
#include "settloader4svaha.h"
#include "cerber4matilda.h"



class SvahaTrymachZjednannya : public QTcpServer
{
    Q_OBJECT
public:
    explicit SvahaTrymachZjednannya(QObject *parent = 0);

    bool startService();

signals:
    void connMe2ThisIdOrMacSig(QStringList, QString);

    void onResourBusy(QString);

    void startConn(QString , int , QString , QString , QString , QString);//server: ip, port, socket id mac <remote ip>:<descr>
    void startConn(QString , int ,  QString );//server: ip, port, socket id mac <remote ip>:<descr>

    void checkThisMac(QString);

    void remoteIdAndDevId(QStringHash , QStringHash , QStringHash , QString );

    void updateCerver();

    void killClientNow(QString id, bool byDevId);



public slots:
    void addMyId2Hash(QString id, QStringList macList, QString remoteId);//id mac <remote ip>:<descr>
    void removeMyId2Hash(QStringList macList);//id mac <remote ip>:<descr>

    void connMe2ThisIdOrMac(QString macOrId, bool isMac, QString myRemoteId, QString rIp);//mac or id, isMacMode, socket id

    void getHashRemoteIdAndDevId(QString id, bool add2auto);

    void removeCerverID(QString id);



protected:
    void incomingConnection(qintptr handle);

private slots:
    void sendCerverInfo();



private:

    QStringHash hashMacRemoteId, hashMacDevId, hashMacAddTime;

    QString server4matildadev, server4matildaConf;
    quint16 svahaServicePort;

    QStringList cerverIdList;

    QTimer tmrCerver;
};

#endif // SVAHATRYMACHZJEDNANNYA_H
