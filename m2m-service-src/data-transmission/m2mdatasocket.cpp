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

#include "m2mdatasocket.h"

M2MDataSocket::M2MDataSocket(QObject *parent) : QTcpSocket(parent)
{
    connect(this, &M2MDataSocket::readyRead, this, &M2MDataSocket::mReadyRead);
    connect(this, &M2MDataSocket::disconnected, this, &M2MDataSocket::onDisconn);
}

void M2MDataSocket::mWrite2socket(QByteArray writeArr)
{
    write(writeArr);
    waitForBytesWritten(5);
}

void M2MDataSocket::onDisconn()
{
    emit iAmDisconn();
    close();
    deleteLater();
}

void M2MDataSocket::mReadyRead()
{
    emit mReadData(readAll());
}
