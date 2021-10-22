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

#ifndef M2MDATASOCKET_H
#define M2MDATASOCKET_H

//former SocketProsto

#include <QTcpSocket>

class M2MDataSocket : public QTcpSocket
{
    Q_OBJECT
public:
    explicit M2MDataSocket(QObject *parent = nullptr);

signals:
    void mReadData(QByteArray readArr);

    void iAmDisconn();

public slots:

    void mWrite2socket(QByteArray writeArr);

    void onDisconn();

private slots:
    void mReadyRead();

private:
//    QString socketId; wtf ???
};

#endif // M2MDATASOCKET_H
