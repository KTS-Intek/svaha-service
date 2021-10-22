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

#ifndef M2MSERVERFORTWO_H
#define M2MSERVERFORTWO_H

//former SvahaDlaDvoh
#include <QTcpServer>
#include <QTimer>
#include <QDebug>


class M2MServerForTwo : public QTcpServer
{
    Q_OBJECT
public:
    explicit M2MServerForTwo(const qint32 &msec4zombie, const qint32 &msecAlive, const bool &verboseMode, QObject *parent = nullptr);

    quint16 findFreePort(const quint16 &minPort, const quint16 &maxPort);

    QTimer *createZombieTimer(const int &msec, const QString &tmrName);
signals:
    void stopAllNow();

    void data2Remote1(QByteArray readArr); //data from the first to the second

    void data2Remote2(QByteArray readArr); //data from the second to the first

protected:
    void incomingConnection(qintptr handle);


public slots:
    void onThreadStarted();

    void onOneDisconn();

    void onZombie();

    void add2TmpBuff(QByteArray buffArr);

private:

    struct MyServiceStateParams
    {
        int connCounter;

        QByteArray buffArr;

        bool verboseMode;
        qint32 msec4zombie;

        MyServiceStateParams() : connCounter(0), verboseMode(false), msec4zombie(60000) {}
    } myStateParams;



};

#endif // M2MSERVERFORTWO_H
