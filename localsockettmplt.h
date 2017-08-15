#ifndef LOCALSOCKETTMPLT_H
#define LOCALSOCKETTMPLT_H

#include <QLocalSocket>

#include <QObject>
#include <QtCore>

#include "moji_defy.h"

class LocalSocketTmplt : public QLocalSocket
{
    Q_OBJECT
public:
    explicit LocalSocketTmplt(QObject *parent = 0);

signals:

    void startReconnTmr();

    void startZombieKiller();
    void stopZombieKiller();

    void startZombieDetect();
    void stopZombieDetect();


    void onConfigChanged(quint16, QVariant);

    void command4dev(quint16,QString);

//default signals
    void reloadSett();
    void killApp();


public slots:
    //for client side
    void initializeSocket(quint16 mtdExtName);
    void command2extensionClient(quint16 command, QVariant dataVar);

    void connect2extension();
    void stopConnection();

    void objInit();

    void setVerboseMode(bool verboseMode);

private slots:
    void mReadyRead();
    void mWrite2extension(const QVariant &s_data, const quint16 &s_command);
    void onDisconn();

    void onZombie();

    void onConnected();


private:
    void decodeReadData(const QVariant &dataVar, const quint16 &command);
    void mReadyReadF();

    quint16 mtdExtName;
    QTime timeHalmo;
    quint8 zombieNow;
    bool stopAll, verboseMode;

    int inConn;
    quint8 reconnCounter;


};

#endif // LOCALSOCKETTMPLT_H
