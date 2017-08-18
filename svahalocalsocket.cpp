#include "svahalocalsocket.h"

//-----------------------------------------------------------------------------------------------
SvahaLocalSocket::SvahaLocalSocket(const bool &verboseMode, QObject *parent) : LocalSocketTmplt(parent)
{
    setVerboseMode(verboseMode);
    initializeSocket(MTD_EXT_NAME_SVAHA_SERVICE);
}
//-----------------------------------------------------------------------------------------------
void SvahaLocalSocket::onConfigChangedSlot(quint16 command, QVariant var)
{
    if(!var.isValid())
        return;

    if(command == MTD_EXT_CUSTOM_COMMAND_3){

        QVariantHash h = var.toHash();
        if(!h.value("sid").toString().isEmpty())
            emit killClientNow(h.value("sid").toString(), false);
        else if(!h.value("id").toString().isEmpty())
            emit killClientNow(h.value("id").toString(), true);


    }
    qDebug() << "onConfigChangedSlot =" << command << var;
}
//-----------------------------------------------------------------------------------------------
void SvahaLocalSocket::onThreadStarted()
{

    connect(this, SIGNAL(onConfigChanged(quint16,QVariant)), this, SLOT(onConfigChangedSlot(quint16,QVariant)) );
    objInit();
}
//-----------------------------------------------------------------------------------------------
