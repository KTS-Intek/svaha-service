#ifndef SVAHALOCALSOCKET_H
#define SVAHALOCALSOCKET_H

#include "localsockettmplt.h"
#include <QObject>

class SvahaLocalSocket : public LocalSocketTmplt
{
    Q_OBJECT

public:
    explicit SvahaLocalSocket(const bool &verboseMode, QObject *parent = 0);


signals:
    void killClientNow(QString id, bool byDevId);


public slots:
    void onConfigChangedSlot(quint16 command, QVariant var);

    void onThreadStarted();

};

#endif // SVAHALOCALSOCKET_H
