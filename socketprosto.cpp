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
#include "socketprosto.h"

//----------------------------------------------------------------------------------

SocketProsto::SocketProsto(QObject *parent) : QTcpSocket(parent)
{
    connect(this, SIGNAL(readyRead()), this, SLOT(mReadyRead()) );
    connect(this, SIGNAL(disconnected()), this, SLOT(onDisconn()) );
}

//----------------------------------------------------------------------------------

void SocketProsto::mWrite2socket(QByteArray writeArr)
{
        write(writeArr);
        waitForBytesWritten(5);
}
//----------------------------------------------------------------------------------

void SocketProsto::onDisconn()
{
    emit iAmDisconn();
    close();
    deleteLater();
}
//----------------------------------------------------------------------------------

void SocketProsto::mReadyRead()
{
    emit mReadData(readAll());
}
//----------------------------------------------------------------------------------
