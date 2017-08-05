#ifndef CHECKLOCALFILESHA1_H
#define CHECKLOCALFILESHA1_H

#include <QObject>

#define DT_MODE_EVERY_DAY   1
#define DT_MODE_EVERY_WEEK  2
#define DT_MODE_EVERY_MONTH 3

/*
 * 1. search for files with mac
 * 2. delete old files with mac
 *
*/


class CheckLocalFileSha1 : public QObject
{
    Q_OBJECT
public:
    explicit CheckLocalFileSha1(const QStringList &macL, const QString &workDir, const quint8 &dtMode, const quint8 &maxFileCount, QObject *parent = 0);

signals:
    void setLocalMacDateSha1(QStringList macL, QStringList sha1L, QList<QDateTime> dtCreatedUtcL, int counter);

    void appendMac2queueSyncRequest(QStringList macL, int counter);//no file or file too old

    void hasRemovedMacs(QStringList macL, int counter);

public slots:
    void onThreadStarted();

private:
    void startCheck();

    QStringList appendThisMac2list(QStringList oldMacList, const QString &fileName);

    struct LclFileSett{
        QStringList macL;
        QString workDir;
        quint8 dtMode;
        quint8 maxFileCount;
    } fsett;

};

#endif // CHECKLOCALFILESHA1_H
