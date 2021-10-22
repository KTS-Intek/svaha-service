#include "m2mlocalsocket.h"

M2MLocalSocket::M2MLocalSocket(bool verboseMode, QObject *parent) : RegularLocalSocket(verboseMode, parent)
{

}

void M2MLocalSocket::decodeReadData(const QVariant &dataVar, const quint16 &command)
{
    //only commands for ntp-bbb
    switch(command){
    //       case MTD_EXT_CUSTOM_COMMAND_0: {
    //           if(verboseMode) qDebug() << "ext " << mtdExtName << dataVar;
    //    #ifdef ENABLE_VERBOSE_SERVER
    //           if(activeDbgMessages)
    //               emit appendDbgExtData(DBGEXT_THELOCALSOCKET, QString("command r: %1, data=%2").arg(command).arg(dataVar.toHash().value("d").toString()));
    //    #endif
    //           emit command4dev(dataVar.toHash().value("c").toUInt(), dataVar.toHash().value("d").toString());
    //           break;}

    case MTD_EXT_COMMAND_RELOAD_SETT: emit reloadSett(); break;
    case MTD_EXT_COMMAND_RESTART_APP: emit killApp()   ;  break;

    case MTD_EXT_CUSTOM_COMMAND_3: onMTD_EXT_CUSTOM_COMMAND_3(dataVar); break;

    default: {
        if(verboseMode)
            qDebug() << "default ext " << command << mtdExtName << dataVar;
        emit onConfigChanged(command,dataVar);
        break;}
    }
}

void M2MLocalSocket::onMTD_EXT_CUSTOM_COMMAND_3(const QVariant &var)
{
    const QVariantHash h = var.toHash();
    //socket id
    if(!h.value("sid").toString().isEmpty()){
        emit killClientNow(h.value("sid").toString(), false);
        return;
    }
    //device id
    if(!h.value("id").toString().isEmpty())
        emit killClientNow(h.value("id").toString(), true);
}
