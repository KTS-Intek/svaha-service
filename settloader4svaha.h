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
#ifndef SETTLOADER4SVAHA_H
#define SETTLOADER4SVAHA_H

#include <QObject>

#define SETT_SVAHA_SERVICE_PORT             1
#define SETT_MATILDA_DEV_IP                 2
#define SETT_MATILDA_CONF_IP                3
#define SETT_SVAHA_DATA_START_PORT          4
#define SETT_SVAHA_DATA_PORT_COUNT          5


#define SETT_ZOMBIE_MSEC                    6
#define SETT_TIME_2_LIVE                    7

#define SETT_ADMIN_LOGIN                    8
#define SETT_ADMIN_PASSWRD                  9


#define SETT_CERBERUS_PORT                  10


#define SETT_SVAHA_MAXIMUM_PENDING_CONN     11


#define SETT_SYNC_WORKDIR                   12
#define SETT_SYNC_MODE                      13
#define SETT_SYNC_MAX_FILE_COUNT            14
#define SETT_SYNC_MAX_SIZE_MAC_TABLE        15
#define SETT_SYNC_MAX_COUNT_SHA1_CHRSPRLL   16
#define SETT_SYNC_MAX_SIZE_SYNC_REQUEST     17
#define SETT_SYNC_MAX_COUNT_SYNQ_RQSTPRLL   18

#define SETT_SYNC_MAX_YEAR_SAVE             19
#define SETT_SYNC_MIN_UNIQ_MAC_FILES        20


typedef QHash<QString,QString> QStringHash;
typedef QHash<QString, QStringHash> QStringHashHash;

class SettLoader4svaha : public QObject
{
    Q_OBJECT
public:
    explicit SettLoader4svaha(QObject *parent = 0);

    void checkDefSett();

    QString valName4key(const int &key);


    QVariant loadOneSett(const int key);

    bool saveOneSett(const int key, const QVariant data2save);

    QString path2sett();


    static quint16 defSETT_SVAHA_SERVICE_PORT();

    static QString defSETT_MATILDA_DEV_IP();

    static QString defSETT_MATILDA_CONF_IP();

    static quint16 defSETT_SVAHA_DATA_START_PORT();

    static quint16 defSETT_SVAHA_DATA_PORT_COUNT();

    static int defSETT_ZOMBIE_MSEC();

    static int defSETT_TIME_2_LIVE();

    static QString defSETT_ADMIN_LOGIN();

    static QString defSETT_ADMIN_PASSWRD();

    static quint16 defSETT_CERBERUS_PORT();

    static quint16 defSETT_SVAHA_MAXIMUM_PENDING_CONN();

    static QString defSETT_SYNC_WORKDIR();
    static quint8 defSETT_SYNC_MODE();
    static quint32 defSETT_SYNC_MAX_FILE_COUNT();
    static quint32 defSETT_SYNC_MAX_SIZE_MAC_TABLE();
    static quint32 defSETT_SYNC_MAX_COUNT_SHA1_CHRSPRLL();
    static quint32 defSETT_SYNC_MAX_SIZE_SYNC_REQUEST();
    static quint32 defSETT_SYNC_MAX_COUNT_SYNQ_RQSTPRLL();

    static qint32 defSETT_SYNC_MAX_YEAR_SAVE();
    static qint32 defSETT_SYNC_MIN_UNIQ_MAC_FILES();

signals:

public slots:
};

#endif // SETTLOADER4SVAHA_H
