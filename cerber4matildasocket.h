/****************************************************************************
**
**   Copyright © 2016-2017 The KTS-INTEK Ltd.
**   Contact: http://www.kts-intek.com.ua
**
**  This file is part of svaha-service-kts.
**
**  svaha-service-kts is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  svaha-service-kts is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with svaha-service-kts.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/
#ifndef CERBER4MATILDASOCKET_H
#define CERBER4MATILDASOCKET_H

#include <QTcpSocket>
#include <QObject>
#include <QTime>
#include <QTimer>
#include "settloader4svaha.h"

class Cerber4matildaSocket : public QTcpSocket
{
    Q_OBJECT
public:
    explicit Cerber4matildaSocket(QObject *parent = 0);

    void writeTmpStamp(QVariantHash hash);

signals:
    void getHashRemoteIdAndDevId(QString, bool);
    void removeCerverID(QString);

    void killClientNow(QString, bool);

public slots:
    void onThrdStarted();

    void remoteIdAndDevId(QStringHash hashMacRemoteId, QStringHash hashMacDevId, QStringHash hashTime, QString id);


private slots:
    void mReadyRead();
    void mWrite2Socket(const QVariantHash s_data, const quint16 s_command);
    void onDisconn();

        //    QStringHash hashMacRemoteId, hashMacDevId;

private:
    void decodeReadData(const QVariantHash &readHash, const quint16 &command);
    bool isConnOpen();
//    QVariantHash errCodeLastOperation(const quint16 &command, const int &errCode) const;


    quint8 authorizeF(QVariantHash h);
    QVariantHash uncompressRead(QByteArray readArr, quint16 &command);

    QTime timeZombie;
    QString myRemoteIpAndDescr;
    bool stopAll;

    QTime timeGalmo;
    qint64 connSpeed;

    bool hiSpeedConnection;

 qint64 lastByteWrt;
    quint8 accessLevel;
    QByteArray tmStamp;



};

#endif // CERBER4MATILDASOCKET_H
