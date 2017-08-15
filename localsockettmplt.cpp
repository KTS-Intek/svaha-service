#include "localsockettmplt.h"

#include <QHostAddress>
#include "settloader4matilda.h"

//-----------------------------------------------------------------------------------------------------------------------------------------------
LocalSocketTmplt::LocalSocketTmplt(QObject *parent) : QLocalSocket(parent)
{
    verboseMode = false;//extended out / verbouseMode
    stopAll = false;
}
//-----------------------------------------------------------------------------------------------------------------------------------------------
void LocalSocketTmplt::initializeSocket(quint16 mtdExtName)
{
    this->mtdExtName = mtdExtName;
}
//-----------------------------------------------------------------------------------------------------------------------------------------------
void LocalSocketTmplt::command2extensionClient(quint16 command, QVariant dataVar)
{
    mWrite2extension(dataVar, command);
}
//-----------------------------------------------------------------------------------------------------------------------------------------------
void LocalSocketTmplt::connect2extension()
{
    reconnCounter++;
    emit stopZombieKiller();
    emit stopZombieDetect();

    if(reconnCounter > 3){
        emit killApp();

        if(reconnCounter > 5){
            int a = 0;
            int b = 10;
            a = b/a;//kill fucking app!!!
        }
    }

    if(state() != QLocalSocket::UnconnectedState){
        close();
        waitForDisconnected();
    }
    stopAll = false;
    zombieNow = 0;
    connectToServer(SettLoader4matilda::defLocalServerName());

    if(waitForConnected(500)){
        emit startZombieDetect();
        if(verboseMode)
            qDebug() << "connect2extension work" << errorString() << state();
        return;
    }
    if(verboseMode)
        qDebug() << "connect2extension not working " << errorString() << state() << mtdExtName;
    stopAll = true;
    emit startReconnTmr();
}
//-----------------------------------------------------------------------------------------------------------------------------------------------
void LocalSocketTmplt::stopConnection()
{
    stopAll = true;
    close();
}
//-----------------------------------------------------------------------------------------------------------------------------------------------
void LocalSocketTmplt::objInit()
{
    reconnCounter = 0;
    connect(this, SIGNAL(readyRead())   , this, SLOT(mReadyRead())  );
    connect(this, SIGNAL(disconnected()), this, SLOT(onDisconn())   );
    connect(this, SIGNAL(connected())   , this, SLOT(onConnected()) );

    QTimer *zombieDetect = new QTimer(this);
    zombieDetect->setSingleShot(true);
    zombieDetect->setInterval(9999);// 10 сек

    connect(this, SIGNAL(startReconnTmr())   , zombieDetect, SLOT(stop()) );
    connect(this, SIGNAL(startZombieDetect()), zombieDetect, SLOT(start()) );
    connect(this, SIGNAL(stopZombieDetect()) , zombieDetect, SLOT(stop()) );
    connect(zombieDetect, SIGNAL(timeout())  , this, SLOT(onZombie()) );


    QTimer *voskresator = new QTimer(this);
    voskresator->setSingleShot(true);
    voskresator->setInterval(333);

    connect(this, SIGNAL(startReconnTmr())   , voskresator, SLOT(start()));
    connect(this, SIGNAL(startZombieKiller()), voskresator, SLOT(stop()) );
    connect(this, SIGNAL(stopZombieKiller()) , voskresator, SLOT(stop()) );
    connect(this, SIGNAL(stopZombieDetect()) , voskresator, SLOT(stop()) );
    connect(this, SIGNAL(startZombieDetect()), voskresator, SLOT(stop()) );
    connect(this, SIGNAL(readyRead())        , voskresator, SLOT(stop()) );

    connect(voskresator, SIGNAL(timeout()), this, SLOT(connect2extension()) );

    connect2extension();
}
//-----------------------------------------------------------------------------------------------------------------------------------------------
void LocalSocketTmplt::setVerboseMode(bool verboseMode)
{
    this->verboseMode = verboseMode;
}
//-----------------------------------------------------------------------------------------------------------------------------------------------
void LocalSocketTmplt::mReadyRead()
{
    disconnect(this, SIGNAL(readyRead()), this, SLOT(mReadyRead()) );
    mReadyReadF();
    connect(this, SIGNAL(readyRead()), this, SLOT(mReadyRead()) );
}
//-----------------------------------------------------------------------------------------------------------------------------------------------
void LocalSocketTmplt::mWrite2extension(const QVariant &s_data, const quint16 &s_command)
{
    if(stopAll){
         if(verboseMode)
             qDebug() << "w stop " << stopAll;
        return ;
    }

    if(state() != QLocalSocket::ConnectedState){
         if(verboseMode)
             qDebug() << "w !state " << state();
        emit startReconnTmr();
        return;
    }

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_6);
    out << (quint32)0;
    out << s_command << s_data;
    out.device()->seek(0);
    out << (quint32)(block.size() - sizeof(quint32));
    write(block);
    waitForBytesWritten(50);
    timeHalmo.restart();
}
//-----------------------------------------------------------------------------------------------------------------------------------------------
void LocalSocketTmplt::onDisconn()
{
    if(verboseMode)
        qDebug() << "onDisconn";
   close();
   stopAll = true;
   emit startReconnTmr();
}
//-----------------------------------------------------------------------------------------------------------------------------------------------
void LocalSocketTmplt::onZombie()
{
    zombieNow++;
    mWrite2extension(QByteArray("h"), MTD_EXT_PING_2_SERV);
    emit startZombieKiller();

    if(zombieNow > 3){
        onDisconn();
    }
}
//-----------------------------------------------------------------------------------------------------------------------------------------------
void LocalSocketTmplt::onConnected()
{
    reconnCounter = 0;
}
//-----------------------------------------------------------------------------------------------------------------------------------------------
void LocalSocketTmplt::decodeReadData(const QVariant &dataVar, const quint16 &command)
{
    switch(command){
    case MTD_EXT_GET_INFO: { mWrite2extension(mtdExtName, MTD_EXT_GET_INFO );  if(verboseMode) qDebug() << "ext " << mtdExtName << dataVar;   break;}

    case MTD_EXT_GET_LOCATION: {  if(verboseMode) qDebug() << "ext " << mtdExtName << dataVar;  break;}
    case MTD_EXT_PING: {  emit startZombieDetect(); emit stopZombieKiller(); break;}

    case MTD_EXT_COMMAND_RELOAD_SETT: emit reloadSett(); break;
    case MTD_EXT_COMMAND_RESTART_APP: emit killApp()   ;  break;

    default: {  if(verboseMode) qDebug() << "default ext " << command << mtdExtName << dataVar; emit onConfigChanged(command,dataVar);  break;}
    }
}
//-----------------------------------------------------------------------------------------------------------------------------------------------
void LocalSocketTmplt::mReadyReadF()
{
    if(state() != QLocalSocket::ConnectedState){
        QTimer::singleShot(11, this, SLOT(onDisconn()));
         if(verboseMode)
             qDebug() << "r !state " << state();

        return;
    }

    zombieNow = 0;
    for(int i = 0; i < 50 && timeHalmo.elapsed() < 35; i++)
        waitForReadyRead(5);
    timeHalmo.restart();

    quint32 blockSize;
    QDataStream inStrm(this);
    quint16 serverCommand;

    inStrm.setVersion(QDataStream::Qt_5_6);

    if(bytesAvailable() < (qint64)sizeof(quint32))
        return;

    inStrm >> blockSize;

    QTime timeG;
    timeG.start();

    int timeOutG = 9999;
    int timeOut = 300;

    while(bytesAvailable() < blockSize && timeG.elapsed() < timeOutG){

        if(waitForReadyRead(timeOut))
            qDebug()<< "readServer1:"<< bytesAvailable() << blockSize;
    }


    if(bytesAvailable() < blockSize || blockSize > MAX_PACKET_LEN || bytesAvailable() > MAX_PACKET_LEN){
        qDebug()<< "readServer:" << blockSize << bytesAvailable() << readAll().toHex();
        return;
    }


    if(bytesAvailable() == blockSize){
        QVariant readVar;
        inStrm >> serverCommand >> readVar;
        decodeReadData(readVar, serverCommand);

    }else{
        if(!inStrm.atEnd()){
            QVariant readVar;
            inStrm >> serverCommand >> readVar;
            decodeReadData(readVar, serverCommand);
            QTimer::singleShot(11, this, SLOT(mReadyRead()) );
            return;
        }
    }
}
//-----------------------------------------------------------------------------------------------------------------------------------------------
