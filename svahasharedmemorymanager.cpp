#include "svahasharedmemorymanager.h"
#include "src/matilda/matildaprotocolhelper.h"
#include "src/matilda/settloader4matilda.h"
#include "src/matilda/moji_defy.h"
#include "src/shared/sharedmemoprotocolhelper.h"
#include "src/shared/sharedmemohelper.h"
//----------------------------------------------------------------------------------------------
SvahaSharedMemoryManager::SvahaSharedMemoryManager(QObject *parent) : QObject(parent)
{

}

//----------------------------------------------------------------------------------------------

void SvahaSharedMemoryManager::onThreadStarted()
{
    hashStatus = SharedMemoProtocolHelper::readFromSharedMemory(MTD_EXT_NAME_SVAHA_SERVICE);
    shmem.setKey(SharedMemoHelper::defSvahaServerMemoName());

    hashStatus.remove("crbr");

    QTimer *tmrAdd2shmemLater = new QTimer(this);
    tmrAdd2shmemLater->setInterval(55);
    tmrAdd2shmemLater->setSingleShot(true);

    connect(this, SIGNAL(startTmrAdd2sharedMemoLater()), tmrAdd2shmemLater, SLOT(start()) );
    connect(tmrAdd2shmemLater, SIGNAL(timeout()), this, SLOT(onTmrAdd2sharedMemoLater()) );

    emit startTmrAdd2sharedMemoLater();

}

//----------------------------------------------------------------------------------------------

void SvahaSharedMemoryManager::setNewHashStatus(QVariantHash h)
{
    hashStatus = h;
    emit startTmrAdd2sharedMemoLater();

}
//----------------------------------------------------------------------------------------------
void SvahaSharedMemoryManager::onTmrAdd2sharedMemoLater()
{
/*
 * keys
 * crbr - cerberus table
*/
    if(!SharedMemoHelper::write2sharedMemory(hashStatus, shmem, SharedMemoHelper::defSvahaServerSemaName()))
        emit startTmrAdd2sharedMemoLater();
}
//----------------------------------------------------------------------------------------------
void SvahaSharedMemoryManager::setNewCerberusIps(QStringHash hashMacRemoteId, QStringHash hashMacDevId, QStringHash hashTime, QStringHashHash hashAboutObject)
{
    emit startTmrAdd2sharedMemoLater();
    QVariantHash h;
    QList<QString> listMac = hashMacRemoteId.keys();
    for(int i = 0, iMax = listMac.size(); i < iMax; i++)
        h.insert( listMac.at(i), QStringList() << hashMacRemoteId.value(listMac.at(i)) << hashMacDevId.value(listMac.at(i)) << hashTime.value(listMac.at(i))
                  << SettLoader4svaha::strFromStrHash(hashAboutObject.value(listMac.at(i))) );

    hashStatus.insert("crbr", h);
    emit startTmrAdd2sharedMemoLater();
}


//----------------------------------------------------------------------------------------------
