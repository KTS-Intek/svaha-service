#include "m2mbackupserver.h"

///[!] svaha-service
#include "m2m-service-src/data-transmission/m2mbackupsocket.h"



//----------------------------------------------------------------------------------------------------------------

M2MBackupServer::M2MBackupServer(bool verboseMode, quint8 backupSessionId, QObject *parent) : QTcpServer(parent)
{
    myParams.verboseMode = verboseMode;
    myParams.backupSessionId = backupSessionId;

    setMaxPendingConnections(1);
    QTimer::singleShot(1 * 60 * 60 * 1000, this, SLOT(onTime2die()) );//відвожу час 1 годину
}

//----------------------------------------------------------------------------------------------------------------

M2MBackupServer::~M2MBackupServer()
{
    onDestrSignl();
}

//----------------------------------------------------------------------------------------------------------------

quint16 M2MBackupServer::findFreePort(const quint16 &minPort, const quint16 &maxPort)
{
//    myParams.connCounter = 0;//.lastHashSumm = 0; // connCounter = 0;
    for(quint16 port = minPort; port < maxPort; port++){
        if(listen(QHostAddress::Any, port))
            return port;
    }
    return 0;
}

//----------------------------------------------------------------------------------------------------------------

void M2MBackupServer::setWrite4aut(const QByteArray &auth, const QString &lastSha1Base64, const QString &workDir)
{
    myParams.auth = auth;
    myParams.lastSha1Base64 = lastSha1Base64;
    myParams.workDir = workDir;

}

//----------------------------------------------------------------------------------------------------------------

void M2MBackupServer::setZombieAndTimeoutMsec(const ConnectionTimeouts &socketTimeouts)//socketcache

{

    myParams.socketTimeouts = socketTimeouts;


}

//----------------------------------------------------------------------------------------------------------------

void M2MBackupServer::incomingConnection(qintptr handle)
{
    if(myParams.verboseMode)
        qDebug() << "M2MBackupServer inConn " << serverPort();

    M2MBackupSocket *socket = new M2MBackupSocket(this);
    connect(this, &M2MBackupServer::stopAllNow, socket, &M2MBackupSocket::onDisconn);

    if(!socket->setSocketDescriptor(handle) || myParams.connCounter > 0 ){
        if(myParams.verboseMode)
            qDebug() << "Service4uploadBackup incomingConnection if(!socket->setSocketDescriptor(socketDescr)){"
                     << socket->errorString() << socket->socketDescriptor()
                     << socket->localAddress() << socket->peerAddress() << myParams.connCounter;
        socket->close();
        socket->deleteLater();
        return;
    }
    myParams.connCounter++;

    if(myParams.verboseMode)
        qDebug() << "Service4uploadBackup onNewConnection " << socket->peerAddress() << socket->peerName() << socket->peerPort();

    socket->setTimeouts(myParams.socketTimeouts);
    socket->createDecoder(myParams.verboseMode,
                          myParams.auth,
                          myParams.lastSha1Base64,
                          myParams.workDir);

    connect(socket, &M2MBackupSocket::mReadData, this, &M2MBackupServer::dataFromRemote);
    connect(socket, &M2MBackupSocket::iAmDisconn, this, &M2MBackupServer::onOneDisconn);
    connect(socket, &M2MBackupSocket::syncDone, this, &M2MBackupServer::syncDone);// onSyncDone);



}

//----------------------------------------------------------------------------------------------------------------

void M2MBackupServer::onThreadStarted()
{
    QTimer *zombieTmr = new QTimer(this);
    zombieTmr->setObjectName("zombieTmr");
    zombieTmr->setInterval(myParams.socketTimeouts.zombieMsec);//15 * 60 * 1000);
    zombieTmr->setSingleShot(true);

    connect(this, SIGNAL(dataFromRemote()), zombieTmr, SLOT(start()) );
    connect(zombieTmr, SIGNAL(timeout()), this, SLOT(onZombie()) );
    connect(this, SIGNAL(stopAllNow()), zombieTmr, SLOT(stop()) );

    emit dataFromRemote();

    if(myParams.verboseMode)
        qDebug() << "M2MBackupServer onThreadStarted " << myParams.backupSessionId ;
}

//----------------------------------------------------------------------------------------------------------------

void M2MBackupServer::onOneDisconn()
{
    if(myParams.connCounter < 0)
        return;

    if(myParams.verboseMode)
        qDebug() << "Service4uploadBackup onOneDisconn ";

    myParams.connCounter = -1;
    killTheConnection("M2MBackupServer::onOneDisconn");
}

//----------------------------------------------------------------------------------------------------------------

void M2MBackupServer::onZombie()
{
    killTheConnection("M2MBackupServer::onZombie");
}

void M2MBackupServer::onTime2die()
{
    killTheConnection("M2MBackupServer::onTime2die");

}

void M2MBackupServer::onDieAfterSync()
{
    killTheConnection("M2MBackupServer::onDieAfterSync");

}

void M2MBackupServer::killTheConnection(QString message)
{
    if(myParams.verboseMode)
        qDebug() << "M2MBackupServer killTheConnection message " << message ;
    myParams.connCounter = -1;
    close();
    emit stopAllNow();

    deleteLater();
}

//----------------------------------------------------------------------------------------------------------------

void M2MBackupServer::onDestrSignl()
{
    emit onSyncServiceDestr(myParams.backupSessionId);

}

//----------------------------------------------------------------------------------------------------------------

void M2MBackupServer::syncDone(QStringList macL, QString lastSha1base64, qint64 msecCreated)
{
    emit onSyncDone(myParams.backupSessionId, macL, lastSha1base64, msecCreated);
    QTimer::singleShot(55, this, SLOT(onDieAfterSync()) );
}

//----------------------------------------------------------------------------------------------------------------
