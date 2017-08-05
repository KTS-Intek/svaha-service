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
#ifndef SVAHADLADVOH_H
#define SVAHADLADVOH_H

#include <QObject>
#include <QTcpServer>

class SvahaDlaDvoh : public QTcpServer
{
    Q_OBJECT
public:
    explicit SvahaDlaDvoh(const bool &verboseMode, QObject *parent = 0);
    quint16 findFreePort(quint16 minP, const quint16 &maxP);

signals:
    void data2Remote(QByteArray); //data
    void data2Remote2(QByteArray); //data


    void stopAllNow();

protected:
    void incomingConnection(qintptr handle);

public slots:
    void onThrdStarted();

    void onOneDisconn();
    void onZombie();

    void add2TmpBuff(QByteArray buffArr);
private:
    int connCounter;
    QByteArray buffArr;

    bool verboseMode;
};

#endif // SVAHADLADVOH_H
