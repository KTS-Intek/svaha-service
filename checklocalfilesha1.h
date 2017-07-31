#ifndef CHECKLOCALFILESHA1_H
#define CHECKLOCALFILESHA1_H

#include <QObject>

#define DT_MODE_EVERY_DAY   1
#define DT_MODE_EVERY_WEEK  2
#define DT_MODE_EVERY_YEAR  3

/*
 * 1. search for files with mac
 * 2. delete old files with mac
 *
*/


class CheckLocalFileSha1 : public QObject
{
    Q_OBJECT
public:
    explicit CheckLocalFileSha1(const LclFileSett &fsett, QObject *parent = 0);

signals:
    void setLocalMacDateSha1(QStringList macL, QList<QByteArray> sha1L, QDateTime dtCreatedUtc, int counter);

    void appendMac2queue(QStringList macL, int counter);//no file or file too old


public slots:
    void onThreadStarted();

private:
    void startCheck();

    struct LclFileSett{
        QStringList macL;
        QString workDir;
        quint8 dtMode;
        quint8 maxFileCount;
    } fsett;

};

#endif // CHECKLOCALFILESHA1_H
