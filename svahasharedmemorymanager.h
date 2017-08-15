#ifndef SVAHASHAREDMEMORYMANAGER_H
#define SVAHASHAREDMEMORYMANAGER_H

#include <QObject>
#include <QtCore>
#include <QSharedMemory>
#include "settloader4svaha.h"

class SvahaSharedMemoryManager : public QObject
{
    Q_OBJECT
public:
    explicit SvahaSharedMemoryManager(QObject *parent = 0);

signals:
    void startTmrAdd2sharedMemoLater();


public slots:
    void onThreadStarted();

    void setNewHashStatus(QVariantHash h);

    void onTmrAdd2sharedMemoLater();

    void setNewCerberusIps(QStringHash hashMacRemoteId, QStringHash hashMacDevId, QStringHash hashTime, QStringHashHash hashAboutObject);


private:
    QVariantHash hashStatus;
    QSharedMemory shmem;
};

#endif // SVAHASHAREDMEMORYMANAGER_H
