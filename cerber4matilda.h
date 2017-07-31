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
#ifndef CERBER4MATILDA_H
#define CERBER4MATILDA_H

#include <QTcpServer>
#include <QObject>
#include "settloader4svaha.h"

class Cerber4matilda : public QTcpServer
{
    Q_OBJECT
public:
    explicit Cerber4matilda(QObject *parent = 0);

    bool startServer(quint16 port);

signals:
    void getHashRemoteIdAndDevId(QString, bool);
    void removeCerverID(QString);

    void remoteIdAndDevId(QStringHash , QStringHash ,QStringHash , QString, QStringHashHash hashAboutObject);

    void killClientNow(QString, bool);

protected:
    void incomingConnection(qintptr handle);

};

#endif // CERBER4MATILDA_H
