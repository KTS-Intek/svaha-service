#ifndef M2MOLDBACKUPSKILLER_H
#define M2MOLDBACKUPSKILLER_H

#include <QObject>
#include <QHash>


//former OldBackupCleaner
class M2MOldBackupsKiller : public QObject
{
    Q_OBJECT
public:
    explicit M2MOldBackupsKiller(const bool &verboseMode, QObject *parent = nullptr);



signals:
    void startTmrCheckMacs(int msec);

public slots:
    void onThreadStarted();

    void setMaxSett(qint32 maxYearSave, qint32 maxMacsSave, qint32 minUniqNumb, QString workDir);

    void onFileCreated(QStringList macL);

    void checkRemovedMacs(const QStringList &macL, const int &counter);

    void stopAllAndDie();

private slots:
    void onCheckMacsTmr();


private:
    void clearOldFile();

    void clearFileByMac();


    int removeTheseFiles(const QStringList &fileNames, const QString &lastPath, const QRegularExpression &re, QHash<QString,qint32> &hMac2counter, QHash<QString,QString> &hMac2oneFileName, QStringList &lRemovePaths);




    struct BackupsKillerParams
    {
        bool verboseMode;

        qint32 maxYearSave;
        qint32 maxMacsSave;
        qint32 minUniqNumb;

        QString workDir;

        QStringList listMacQueue;
        QHash<QString,bool> hMacQueue;

        quint16 clearOldFileCounter; //WTF is that ????
        BackupsKillerParams() : verboseMode(false), maxYearSave(1000), maxMacsSave(1000), minUniqNumb(1000), clearOldFileCounter(0) {}
    } myParams;



};

#endif // M2MOLDBACKUPSKILLER_H
