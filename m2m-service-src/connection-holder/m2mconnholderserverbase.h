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
#ifndef M2MCONNHOLDERSERVERBASE_H
#define M2MCONNHOLDERSERVERBASE_H


#include <QTcpServer>

///[!] svaha-service
//#include "m2m-service-src/main/m2mservertypes.h"
#include "m2m-service-src/connection-holder/m2mconnholdersocket.h"


//base processing methods are here


class M2MConnHolderServerBase : public QTcpServer
{
    Q_OBJECT
public:
    explicit M2MConnHolderServerBase(const bool &verboseMode, QObject *parent = nullptr);

    struct ConnHolderServerParams
    {
        bool verboseMode;
        quint16 port;

        qint32 connectionLimit;

        ConnectionTimeouts socketTimeouts;//socketcache

        QString workDir; //backup workDir

//data connection
        QString serverDataIP;
//        quint16 serverPort;//65000
        quint16 serverDataStart; //50000
        quint16 serverDataEnd; //60000

        ConnHolderServerParams() : connectionLimit(1000) {}
    } myParams;


    struct ConnHolderServerTable
    {
        QStringHash hashMacRemoteId;
        QStringHash hashMacDevId;
        QStringHash hashMacAddTime;
        QStringHashHash hashMac2objectIfo;

        qint32 connectionCounter;

        ConnHolderServerTable() : connectionCounter(0) {}
    } myTable;

    static void makeCustomTypeRegistration();

    bool startService();

    QStringList getDevicesWithThisID(const QString &id);// id, socket id

    QStringList getDevicesWithThisMAC(const QString &macUpper);//mac , socket id

    bool canAllowOneMoreSocket();

signals:


    void onFailed2startServer(QString message);

    void addEvent2log(QString message);

    //connection holder sockets to BackUpManager
    void checkBackup4thisMac(QString mac, QString lastSha1base64);//check SHA1 for last backup file and if not equal: create new backup (check settings before this)  and upload to service

    void onSyncDone(quint8 sessionId, QStringList macL, QString lastSha1base64, QDateTime dtCreatedUtc);//на віддаленому пристрої ХЕШ сума файлу змінилась, завантаження здійснено

    void onSyncServiceDestr(quint8 sessionId);

    //to connection decoder
    void checkThisMac(QString mac);

    void onResourBusy(QString socketId);


    void startStopSuicideTmr(QString remIpDescr, bool start);


    //to connection holder sockets
    //former connMe2ThisIdOrMacSlot
    void onFailed2connect2oneDev(QStringList macIdList, QString socketId);//no device or more than one

    void startConnMatildaDev(quint16 serverPort, QString objId, QString objMac, QString objSocketId, QString rIp);//server: ip, port, socket id mac <remote ip>:<descr>

    void startConn4UCon(quint16 serverPort,  QString objSocketId);//server: ip, port, socket id mac <remote ip>:<descr>


    //on settings changed
    void onDataConnectionParamsChanged(QString serverDataIP, quint16 serverDataStart, quint16 serverDataEnd);

    void onBackupWorkDirectoryChanged(QString workDir);

    void onTimeoutsChanged(ConnectionTimeouts socketTimeouts);

//former setNewCerberusIps, to shared memory manager
    void onConnectionTableChanged();

    void onConnectionTableData(QStringHash hashMacRemoteId, QStringHash hashMacDevId, QStringHash hashTime, QStringHashHash hashAboutObject);


    void killAllClinets();


    //BackUpManager
    //to manager
    void onSyncRequestRemoteSha1isEqual(QStringList macL);//на віддаленому пристрої ХЕШ сума файлу не змінилась, не чіпаю, тільки видаляю з черги wait4answerSyncQueue
    void onSyncFileDownloaded(QStringList macL, QString lastSha1base64, qint64 msecCreated);//QDateTime dtCreatedUtc);//на віддаленому пристрої ХЕШ сума файлу змінилась, завантаження здійснено


    //onConnectedThisMacs
    void onConnectedTheseMacs(QStringList macL); //only if allowed sync, check mac, add 2 checkSha1LocalFsMacQueue
    //onDisconnectedThisMacs
    void onDisconnectedTheseMacs(QStringList macL, int counter);



public slots:
    void stopAllSlot();

    //settings
    void setTimeouts(int zombieMsec, int msecAlive, int timeOutGMsec, int timeOutBMsec);

    void setBackupWorkDirectory(QString workDir);

    void setDataConnectionParams(QString serverDataIP, quint16 minDataPort, quint16 maxDataPort);

//    void sendAllSettings();


    //from connection decoder
    void addMyId2Hash(QString objId, QStringList macl, QString remIpDescr, QStringHash hashObjIfo, bool add2sync);//id mac <remote ip>:<descr>

    void removeMyId2Hash(QStringList macl, QString remIpDescr);//id mac <remote ip>:<descr>

    void setInfoAboutObj(QStringList macL, QStringHash objIfo, int counter);







    void tellThatConnectionTableChanged();

    void sendConnectionTable();


    void onThisDecoderReadyBase(M2MConnHolderDecoder *decoder);

    void setConnectionLimit(int connectionLimit);

    void onSocketDied();
};

#endif // M2MCONNHOLDERSERVERBASE_H
