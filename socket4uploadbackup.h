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

#ifndef SOCKET4UPLOADBACKUP_H
#define SOCKET4UPLOADBACKUP_H

#include <QObject>
#include <QTcpSocket>
#include <QDateTime>


class Socket4uploadBackup : public QTcpSocket
{
    Q_OBJECT
public:
    explicit Socket4uploadBackup(const bool &verboseMode, QString write4authorizeBase64, QString lastSha1base64, QObject *parent = 0);

signals:

    void mReadData();

    void iAmDisconn();

    void onSyncDone(QString lastSha1base64, QDateTime dtCreatedUtc);//на віддаленому пристрої ХЕШ сума файлу змінилась, завантаження здійснено

public slots:


    void onDisconn();

private slots:
    void mReadyRead();

private:
    void mReadyReadF();

    void decodeReadData(const QVariant &dataVar, const quint16 &command);

    void decodeReadDataJSON(const QByteArray &dataArr);

    bool messHshIsValid(const QJsonObject &jObj, QByteArray readArr);

    QStringList getHshNames() const;
    qint64 mWrite2SocketJSON(QJsonObject jObj, const quint16 s_command, int lastHashSumm);

    QVariant uncompressRead(QByteArray readArr, quint16 &command, qint64 lenBefore);

    QByteArray varHash2arr(const QVariantHash &hash);

    void mWriteToSocket(const QVariant s_data, const quint16 s_command);


    void saveBackupArrAsFile();

    QString fileNameFromAboutObject();


    bool verboseMode;

    QString socketId;
    QString write4authorizeBase64;

    QByteArray backupArr;
    qint32 backupArrLen;
    QDateTime dtCreatedBackupUtc;

    bool matildaLogined;
    int dataStreamVersion;
    bool stopAfter, stopAll;
    quint8 zombieRetr;
    quint8 accessLevel;

    QVariantHash hashAboutObj;
    QString lastSha1base64;



};

#endif // SOCKET4UPLOADBACKUP_H
