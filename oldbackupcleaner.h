#ifndef OLDBACKUPCLEANER_H
#define OLDBACKUPCLEANER_H

#include <QObject>
#include <QHash>

class OldBackupCleaner : public QObject
{
    Q_OBJECT
public:
    explicit OldBackupCleaner(const qint32 &maxYearSave, const qint32 &maxMacsSave, const qint32 &minUniqNumb, const QString &workDir, QObject *parent = 0);

signals:
    void startTmrCheckMacs(int msec);

public slots:
    void onThreadStarted();

    void setMaxSett(qint32 maxYearSave, qint32 maxMacsSave, qint32 minUniqNumb, QString workDir);

    void onFileCreated(QStringList macL);

    void checkRemovedMacs(QStringList macL, int counter);


private slots:
    void onCheckMacsTmr();


private:
    void clearOldFile();

    void clearFileByMac();

    QStringList getMacsFromFileName(int &counter, const QString &fileName);



    qint32 maxYearSave;
    qint32 maxMacsSave;
    qint32 minUniqNumb;

    QString workDir;

    QStringList listMacQueue;
    QHash<QString,bool> hMacQueue;

    quint16 clearOldFileCounter;


};

#endif // OLDBACKUPCLEANER_H
