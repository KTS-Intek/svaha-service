#include "service4uploadbackup.h"
#include <QTimer>

#include "settloader4svaha.h"


//----------------------------------------------------------------------------------
Service4uploadBackup::Service4uploadBackup(quint8 backupSessionId, QObject *parent) : QTcpServer(parent)
{
    this->backupSessionId = backupSessionId;
    setMaxPendingConnections(1);
    QTimer::singleShot(12 * 60 * 60 * 1000, this, SLOT(onZombie()) );
}
//----------------------------------------------------------------------------------
quint16 Service4uploadBackup::findFreePort(quint16 minP, const quint16 &maxP)
{
    connCounter = 0;
    for( ; minP < maxP; minP++){
        if(listen(QHostAddress::Any, minP))
            return minP;
    }
    return 0;
}
//----------------------------------------------------------------------------------
void Service4uploadBackup::setWrite4aut(QByteArray a)
{
    write4auth = a;
}
//----------------------------------------------------------------------------------
void Service4uploadBackup::incomingConnection(qintptr handle)
{
    if(verboseMode)
        qDebug() << "inConn " << serverPort();
    SocketProsto *socket = new SocketProsto;
    connect(this, SIGNAL(stopAllNow()), socket, SLOT(onDisconn()) );
    if(!socket->setSocketDescriptor(handle) || connCounter > 1 ){

        if(verboseMode)
            qDebug() << "SvahaDlaDvoh incomingConnection if(!socket->setSocketDescriptor(socketDescr)){" << socket->errorString() << socket->socketDescriptor() << socket->localAddress() << socket->peerAddress() << connCounter;
        socket->close();
        socket->deleteLater();
        return;
    }
    if(verboseMode)
        qDebug() << "SvahaDlaDvoh onNewConnection " << socket->peerAddress() << socket->peerName() << socket->peerPort();

    connCounter++;
    if(connCounter == 1){//перший
        connect(socket, SIGNAL(mReadData(QByteArray)), this, SIGNAL(data2Remote2(QByteArray)) );
        connect(this, SIGNAL(data2Remote(QByteArray)), socket, SLOT(mWrite2socket(QByteArray)));
        connect(this, SIGNAL(data2Remote2(QByteArray)), this, SLOT(add2TmpBuff(QByteArray)) );
    }else{
        disconnect(this, SIGNAL(data2Remote2(QByteArray)), this, SLOT(add2TmpBuff(QByteArray)) );

        connect(socket, SIGNAL(mReadData(QByteArray)), this, SIGNAL(data2Remote(QByteArray)) );
        connect(this, SIGNAL(data2Remote2(QByteArray)), socket, SLOT(mWrite2socket(QByteArray)));

        if(!buffArr.isEmpty()){
            socket->mWrite2socket(buffArr);
//            emit data2Remote2(buffArr);
            buffArr.clear();
        }


    }

    connect(socket, SIGNAL(iAmDisconn()), this, SLOT(onOneDisconn()) );
}
//----------------------------------------------------------------------------------
void Service4uploadBackup::onThrdStarted()
{
    connect(this, SIGNAL(destroyed(QObject*)), this, SLOT(onDestrSignl()) );
    SettLoader4svaha sLoader;
    QTimer *zombieTmr = new QTimer(this);
    zombieTmr->setObjectName("zombieTmr");
    zombieTmr->setInterval(sLoader.loadOneSett(SETT_ZOMBIE_MSEC).toInt() );//15 * 60 * 1000);
    zombieTmr->setSingleShot(true);
    connect(this, SIGNAL(dataFromRemote()), zombieTmr, SLOT(start()) );
    connect(zombieTmr, SIGNAL(timeout()), this, SLOT(onZombie()) );
    connect(this, SIGNAL(stopAllNow()), zombieTmr, SLOT(stop()) );
    zombieTmr->start();

}
//----------------------------------------------------------------------------------
void Service4uploadBackup::onOneDisconn()
{
    if(connCounter < 0)
        return;

    if(verboseMode)
        qDebug() << "SvahaDlaDvoh onOneDisconn ";

    connCounter = -1;
    onZombie();
}
//----------------------------------------------------------------------------------
void Service4uploadBackup::onZombie()
{
    if(verboseMode)
        qDebug() << "SvahaDlaDvoh onZombie " ;
    connCounter = -1;
    close();
    emit stopAllNow();

    deleteLater();
}

//----------------------------------------------------------------------------------

void Service4uploadBackup::onDestrSignl()
{
    emit onSyncServiceDestr(backupSessionId);

}
//----------------------------------------------------------------------------------
