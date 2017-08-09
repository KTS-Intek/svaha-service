#include "service4uploadbackup.h"
#include <QTimer>

#include "settloader4svaha.h"
#include "socket4uploadbackup.h"

//----------------------------------------------------------------------------------
Service4uploadBackup::Service4uploadBackup(bool verboseMode, quint8 backupSessionId, QObject *parent) : QTcpServer(parent)
{
    this->verboseMode = verboseMode;
    this->backupSessionId = backupSessionId;
    setMaxPendingConnections(1);
    QTimer::singleShot(12 * 60 * 60 * 1000, this, SLOT(onZombie()) );//відвожу час 12 годин
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
void Service4uploadBackup::setWrite4aut(QByteArray a, QString lastSha1Base64)
{
    if(verboseMode)
        qDebug() << "setWrite4aut=" << a;
    write4auth = a;
    this->lastSha1Base64 = lastSha1Base64;
}
//----------------------------------------------------------------------------------
void Service4uploadBackup::incomingConnection(qintptr handle)
{
    if(verboseMode)
        qDebug() << "Service4uploadBackup inConn " << serverPort();
    Socket4uploadBackup *socket = new Socket4uploadBackup(verboseMode, write4auth, lastSha1Base64, this);
    connect(this, SIGNAL(stopAllNow()), socket, SLOT(onDisconn()) );
    if(!socket->setSocketDescriptor(handle) || connCounter > 0 ){
        if(verboseMode)
            qDebug() << "Service4uploadBackup incomingConnection if(!socket->setSocketDescriptor(socketDescr)){" << socket->errorString() << socket->socketDescriptor() << socket->localAddress() << socket->peerAddress() << connCounter;
        socket->close();
        socket->deleteLater();
        return;
    }
    connCounter++;

    if(verboseMode)
        qDebug() << "Service4uploadBackup onNewConnection " << socket->peerAddress() << socket->peerName() << socket->peerPort();

    connect(socket, SIGNAL(mReadData() ), this, SIGNAL(dataFromRemote()) );
    connect(socket, SIGNAL(iAmDisconn()), this, SLOT(onOneDisconn()) );
    connect(socket, SIGNAL(onSyncDone(QStringList,QString,QDateTime)), this, SLOT(syncDone(QStringList,QString,QDateTime)) );
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

    emit dataFromRemote();

    if(verboseMode)
        qDebug() << "Service4uploadBackup onThrd " ;

}
//----------------------------------------------------------------------------------
void Service4uploadBackup::onOneDisconn()
{
    if(connCounter < 0)
        return;

    if(verboseMode)
        qDebug() << "Service4uploadBackup onOneDisconn ";

    connCounter = -1;
    onZombie();
}
//----------------------------------------------------------------------------------
void Service4uploadBackup::onZombie()
{
    if(verboseMode)
        qDebug() << "Service4uploadBackup onZombie " ;
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
void Service4uploadBackup::syncDone(QStringList macL, QString lastSha1base64, QDateTime dtCreatedUtc)
{

    emit onSyncDone(backupSessionId, macL, lastSha1base64, dtCreatedUtc);
    QTimer::singleShot(55, this, SLOT(onZombie()) );
}
//----------------------------------------------------------------------------------
