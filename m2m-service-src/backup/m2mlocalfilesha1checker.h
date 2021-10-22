#ifndef M2MLOCALFILESHA1CHECKER_H
#define M2MLOCALFILESHA1CHECKER_H

#include <QObject>
#include <QDateTime>
#include <QList>


/*
 * 1. search for files with mac
 * 2. delete old files with mac
 *
*/

//former CheckLocalFileSha1


class M2MLocalFileSha1Checker : public QObject
{
    Q_OBJECT
public:
    explicit M2MLocalFileSha1Checker(const bool &verboseMode, QObject *parent = nullptr);


    struct LclFileSett{
        bool verboseMode;

        QStringList macL;
        QString workDir;
        quint8 dtMode;
        quint8 maxFileCount;

        LclFileSett() : verboseMode(false), dtMode(0xFF), maxFileCount(0xFF) {}

    } fsett;


    void setSettings(const QStringList &macL, const QString &workDir, const quint8 &dtMode, const quint8 &maxFileCount);

    int findTheseMacs(const QStringList &fileNames, const QString &lastPath, const int &nMax, const QRegularExpression &re, const QStringList &macL,
                      QHash<QString,qint32> &hMac2counter, QStringList &lfoundMac, QStringList &lfoundSha1,
                      QList<qint64> &msecCreatedL, int &counterCheckRemoveMacs, QStringList &lCheckMac4remove);

signals:
    void setLocalMacDateSha1(QStringList macL, QStringList sha1L, QList<qint64> msecCreatedL, int counter);

    void appendMac2queueSyncRequest(QStringList macL, int counter);//no file or file too old

    void checkRemovedMacs(QStringList macL, int counter);



public slots:
    void onThreadStarted();


private:
    void startCheck();




};

#endif // M2MLOCALFILESHA1CHECKER_H
