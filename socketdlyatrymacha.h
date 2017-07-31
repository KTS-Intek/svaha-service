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
#ifndef SOCKETDLYATRYMACHA_H
#define SOCKETDLYATRYMACHA_H

#include <QObject>
#include <QTcpSocket>
#include <QStringList>
#include <QJsonObject>
#include <QTime>
#include <QTimer>

#include "settloader4svaha.h"
/*
 * Є два режими
 * 1. Від присторю опитування - при з’єданні очікує на отримання ІД та МАК адреси
 * 2. Від ПЗ конфігурації - при з’єднанні очікує на отримання ІД або МАК адреси клієнта з яким необхідно утворити пару
 *
 *
 *
 */


class SocketDlyaTrymacha : public QTcpSocket
{
    Q_OBJECT
public:
    explicit SocketDlyaTrymacha(const bool &verbouseMode, QObject *parent = 0);

signals:
    void addMyId2Hash(QString objId, QStringList mac, QString remIpDescr, QStringHash hashObjIfo);//id mac <remote ip>:<descr>
    void removeMyId2Hash(QStringList);//id mac


    void connMe2ThisIdOrMac(QString, bool, QString, QString);//mac or id, isMacMode, socket id

    void showMess(QString);
    void infoAboutObj(QString remIpDescr, QStringHash objIfo);




public slots:
    void onThrdStarted();

    void startConn(QString serverIp, int serverPort, QString objId, QString objMac, QString objSocketId, QString rIp);//server: ip, port, socket id mac <remote ip>:<descr>
    void startConn(QString serverIp, int serverPort,  QString objSocketId);//server: ip, port, socket id mac <remote ip>:<descr>

    void connMe2ThisIdOrMacSlot(QStringList macIdList, QString socketId);//<mac>@<id>

    void onResourBusy(QString socketId);

    void checkThisMac(QString mac);


    void killClientNow(QString id, bool byDevId);

private slots:
    void mReadyRead();
    void mWrite2SocketJSON(QJsonObject jObj, const quint16 s_command);
    void onDisconn();



private:
    void mReadyReadF();

    void decodeReadDataJSON(const QByteArray &readArr);
    bool isConnOpen();
    bool messHshIsValid(const QJsonObject &jObj, QByteArray readArr);

    QStringHash getObjIfo(const QVariantMap &h, const bool &addVersion);

    QJsonArray arrFromList(const QStringList &list);
    QString hshSummName(const int &indx) const;
    QStringList getHshNames() const;
    QJsonObject errCodeLastOperationJson(const quint16 &command, const int &errCode) const;
    QVariantHash map2hash(const QVariantMap &map);


    QTime timeZombie;
    int isMatildaDev; //-1 unknown, 0 - Matilda-conf, 1 - matilda-dev
    bool allowCompress;
    QString myRemoteIpAndDescr;
    QStringList mMac;
    QString mIden;
    int lastHashSumm;
    bool stopAll;

    quint16 lastCommand;


    bool verbouseMode;

};

#endif // SOCKETDLYATRYMACHA_H
