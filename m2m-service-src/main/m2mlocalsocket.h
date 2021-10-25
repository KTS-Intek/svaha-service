#ifndef M2MLOCALSOCKET_H
#define M2MLOCALSOCKET_H

///[!] ipc
#include "localsockets/regularlocalsocket.h"

//former LocalSocketTmplt

class M2MLocalSocket : public RegularLocalSocket
{
    Q_OBJECT
public:
    explicit M2MLocalSocket(bool verboseMode, QObject *parent = nullptr);

    void decodeReadData(const QVariant &dataVar, const quint16 &command);

    void onMTD_EXT_CUSTOM_COMMAND_3(const QVariant &var);

signals:
//    void killClientNow(QString id, bool byDevId);
    void killClientsNow(QStringList ids, bool byDevId); //v11


    //default signals
    void reloadSett();
    void killApp();

};

#endif // M2MLOCALSOCKET_H
