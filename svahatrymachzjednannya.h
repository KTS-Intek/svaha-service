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

    void remoteIdAndDevId(QStringHash hashMacRemoteId, QStringHash hashMacDevId, QStringHash hashTime, QString id, QStringHashHash hashAboutObject);

    void updateCerver();

    void killClientNow(QString id, bool byDevId);


    //reload settings timer
    void tmrReloadSettStart();


    //BackUpManager
    //to manager
    void onSyncRequestRemoteSha1isEqual(QStringList macL);//на віддаленому пристрої ХЕШ сума файлу не змінилась, не чіпаю, тільки видаляю з черги wait4answerSyncQueue
    void onSyncFileDownloaded(QStringList macL, QString lastSha1base64, QDateTime dtCreatedUtc);//на віддаленому пристрої ХЕШ сума файлу змінилась, завантаження здійснено

    void onConnectedThisMacs(QStringList macL); //only if allowed sync, check mac, add 2 checkSha1LocalFsMacQueue
    void onDisconnectedThisMacs(QStringList macL, int counter);

//from manager
    void checkBackup4thisMac(QString mac, QString lastSha1base64);//check SHA1 for last backup file and if not equal: create new backup (check settings before this)  and upload to service

    //shared memory manager
    void setNewCerberusIps(QStringHash hashMacRemoteId, QStringHash hashMacDevId, QStringHash hashTime, QStringHashHash hashAboutObject);

//to all
    void checkSett2all();

public slots:
    void addMyId2Hash(QString objId, QStringList mac, QString remIpDescr, QStringHash hashObjIfo, bool add2sync);//id mac <remote ip>:<descr>

    void removeMyId2Hash(QStringList macList);//id mac <remote ip>:<descr>

    void connMe2ThisIdOrMac(QString macOrId, bool isMac, QString myRemoteId, QString rIp);//mac or id, isMacMode, socket id

    void getHashRemoteIdAndDevId(QString id, bool add2auto);

    void removeCerverID(QString id);

    void reloadSettings();

    void initObjects();

    void infoAboutObj(QStringList macL, QStringHash objIfo, int counter);


    void killApp();

protected:
    void incomingConnection(qintptr handle);

private slots:
    void sendCerverInfo();

    void createManagers();



private:
//поточний стан
    QStringHash hashMacRemoteId, hashMacDevId, hashMacAddTime;
    QStringHashHash hashMac2objectIfo;
    bool verboseOut;
    bool createObjects;
//історія підключень


    QString server4matildadev, server4matildaConf;
    quint16 svahaServicePort;

    QStringList cerverIdList;

    QTimer tmrCerver;
};

#endif // SVAHATRYMACHZJEDNANNYA_H
