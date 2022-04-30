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
#ifndef M2MCONNHOLDERSERVER_H
#define M2MCONNHOLDERSERVER_H


///[!] svaha-service
#include "m2m-service-src/connection-holder/m2mconnholderserverbase.h"


///[!] MatildaIO
#include "matilda-bbb-src/shared/ipaccessmanager.h"



class M2MConnHolderServer : public M2MConnHolderServerBase
{
    Q_OBJECT
public:
    explicit M2MConnHolderServer(const bool &verboseMode, QObject *parent = nullptr);

    IPAccessManager *accesManager;

signals:

    void gimmeSettings();

    //to     IPAccessManager
    void setAllowAndBlockList(QStringList allowIpList, QStringList blockThisIp);
    void blockThisIP(QString ip);

//from M2MLocalSocket
//    void killClientNow(QString id, bool byDevId);
    void killClientsNow(QStringList ids, bool byDevId); //v11




public slots:
    void onThreadStarted();

    void try2startServer();

    void setServicePortSmart(quint16 port);
    void setServicePortAndLimitsSmart(quint16 port, int connectionLimit);


    void connMe2ThisIdOrMac(QString macOrId, bool isMac, QString myRemoteId, QString rIp);//mac or id, isMacMode, socket id



    void onThisDecoderReady(M2MConnHolderDecoder *decoder);

private slots:


protected:
    void incomingConnection(qintptr handle);





};

#endif // M2MCONNHOLDERSERVER_H
