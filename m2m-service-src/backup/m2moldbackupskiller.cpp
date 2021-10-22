#include "m2moldbackupskiller.h"



#include <QTimer>
#include <QDir>
#include <QDateTime>
#include <QDebug>

///[!] svaha-service
#include "m2m-service-src/main/m2mglobalmethods.h"


M2MOldBackupsKiller::M2MOldBackupsKiller(const bool &verboseMode, QObject *parent) : QObject(parent)
{
    myParams.verboseMode = verboseMode;
}


void M2MOldBackupsKiller::onThreadStarted()
{
    QTimer *t = new QTimer(this);
    t->setSingleShot(true);
    connect(this, SIGNAL(startTmrCheckMacs(int)), t, SLOT(start(int)) );
    connect(t, SIGNAL(timeout()), this, SLOT(onCheckMacsTmr()) );

    emit startTmrCheckMacs(999);
}

void M2MOldBackupsKiller::setMaxSett(qint32 maxYearSave, qint32 maxMacsSave, qint32 minUniqNumb, QString workDir)
{
    if(maxMacsSave < 0)
        maxMacsSave = 0;
    myParams.maxMacsSave = maxMacsSave;
    myParams.maxYearSave = maxYearSave;
    myParams.workDir = workDir;
    if(minUniqNumb > maxMacsSave)
        minUniqNumb = maxMacsSave;
    if(minUniqNumb < 1)
        minUniqNumb = 1;
    myParams.minUniqNumb = minUniqNumb;


}

void M2MOldBackupsKiller::onFileCreated(QStringList macL)
{
    checkRemovedMacs(macL, macL.size());

}

void M2MOldBackupsKiller::checkRemovedMacs(const QStringList &macL, const int &counter)
{
    for(int i = 0; i < counter; i++){
        const QString mac = macL.at(i);

        if(myParams.hMacQueue.contains(mac))
            continue;
        myParams.listMacQueue.append(mac);
        myParams.hMacQueue.insert(mac, true);
    }
}

void M2MOldBackupsKiller::stopAllAndDie()
{
    deleteLater();
}

void M2MOldBackupsKiller::onCheckMacsTmr()
{
    if(myParams.maxYearSave > 0)
        clearOldFile();

    if(myParams.maxMacsSave > 0)
        clearFileByMac();

    emit startTmrCheckMacs( myParams.listMacQueue.isEmpty() ? 555 : 11);
}

void M2MOldBackupsKiller::clearOldFile()
{
    //<work dir>/<year>/<month>/<file names>  //UTC date time!!!
    //file name <base sha1>_<mac(X)>_...other keys

    if(myParams.workDir.isEmpty())
        return;
    const QString workDir = myParams.workDir;

//    if(myParams.clearOldFileCounter++ != 0)//щоб не так часто
//        return;

    QDir dir(workDir);
    if(!dir.exists())
        return;

    QDate dt = QDateTime::currentDateTimeUtc().date();
    dt.setDate(dt.year(), dt.month(), 1);
    dt = dt.addYears((-1) * myParams.maxYearSave);

    const QDate dtMin = QDateTime(QDate(2017, 1, 1), QTime(0,0,0,0), Qt::UTC).addYears(-10).date();

    for(int j = 0; dt > dtMin && j < 10000; j++){
        const QString lastPath = QString("%1/%2/%3").arg(workDir).arg(dt.year()).arg(dt.month());

        dir.setPath(lastPath);
        if(!dir.exists())
            continue;

        const bool r = dir.removeRecursively();
        if(myParams.verboseMode)
            qDebug() << "remove dir " << r << dir.path() ;
        dt = dt.addMonths(-1);
    }

}

void M2MOldBackupsKiller::clearFileByMac()
{
    //<work dir>/<year>/<month>/<file names>  //UTC date time!!!
    //file name <base64 ( omit "=",  replace("/", "=")) sha1>_<mac(X)>_...other keys
    if(myParams.workDir.isEmpty())
        return;

    const QString workDir = myParams.workDir;

    if(myParams.listMacQueue.isEmpty())
        return;

    QDir dir(workDir);
    if(!dir.exists())
        return;

    QStringList macL;
    for(int i = 0; i < 30 && !myParams.listMacQueue.isEmpty(); i++){
        myParams.hMacQueue.remove(myParams.listMacQueue.first());
        macL.append(myParams.listMacQueue.takeFirst());
    }

    const QRegularExpression re(M2MGlobalMethods::getPattern4thisMacL(macL));

    QDate dt = QDateTime::currentDateTimeUtc().date();
    dt.setDate(dt.year(), dt.month(), 1);
    QDate dtMin = dt.addYears((-1) * myParams.maxYearSave);// QDateTime(QDate(2017, 1, 1), QTime(0,0,0,0), Qt::UTC).date();

    QHash<QString,qint32> hMac2counter;
    //всовую пару МАК назва файлу, якщо в МАК повторюється видаляю пару, тобто якщо буде лише один файл з МАКом,
    //то він залишиться в контейнері, при видаленні файлів буде заборона на видалення файлів з цього контейнера
    QHash<QString,QString> hMac2oneFileName;

    QStringList lRemovePaths;
    QStringList lEmptyDirectory;

    int counter = 0;



    for(int j = 0; dt > dtMin && j < 10000; j++){

        const QString lastPath = QString("%1/%2/%3").arg(workDir).arg(dt.year()).arg(dt.month());

        dir.setPath(lastPath);
        dt = dt.addMonths(-1);

        const QStringList fileNames = dir.entryList(QDir::Files|QDir::NoDotAndDotDot|QDir::NoSymLinks|QDir::Readable, QDir::Time);
        if(fileNames.isEmpty()){
            if(dir.exists())
                lEmptyDirectory.append(lastPath);
            continue;
        }


        counter += removeTheseFiles(fileNames, lastPath, re, hMac2counter, hMac2oneFileName, lRemovePaths);

    }



    const QList<QString> lk = hMac2oneFileName.keys();
    QHash<QString,bool> hIgnoreFileNames;
    for(int i = 0, iMax = lk.size(); i < iMax; i++)
        hIgnoreFileNames.insert(hMac2oneFileName.value(lk.at(i)), true);

    hMac2oneFileName.clear();
//    lk.clear();
    hMac2counter.clear();

    for(int i = 0, iMax = counter; i < iMax; i++){
        if(hIgnoreFileNames.value(lRemovePaths.at(i), false))
            continue;

        const bool r = dir.remove(lRemovePaths.at(i));
        if(myParams.verboseMode)
            qDebug() << "remove file " << r << lRemovePaths.at(i);

    }

    for(int i = 0, imax = lEmptyDirectory.size(); i < imax; i++){
        dir.setPath(lEmptyDirectory.takeFirst());
        const bool r = dir.removeRecursively();
        if(myParams.verboseMode)
            qDebug() << "remove dir0 " << r << dir.path() ;
    }



}

int M2MOldBackupsKiller::removeTheseFiles(const QStringList &fileNames, const QString &lastPath, const QRegularExpression &re, QHash<QString, qint32> &hMac2counter, QHash<QString, QString> &hMac2oneFileName, QStringList &lRemovePaths)
{
    int counter = 0;
    for(int i = 0, iMax = fileNames.size(); i < iMax && i < 1000000; i++){
        const QString s = fileNames.at(i);
        if(s.contains(re)){
            //found matches
            const QString path = lastPath + "/" + s;
            int nMax;
            const QStringList macLtmp = M2MGlobalMethods::getMacsFromFileName(nMax, s);

            for(int n = 0; n < nMax && n < 100; n++){
                if(!hMac2counter.contains(macLtmp.at(n))){
                    //first
                    hMac2oneFileName.insert(macLtmp.at(n), path);
                    hMac2counter.insert(macLtmp.at(n), 1);
                    continue;
                }

                qint32 v = hMac2counter.value(macLtmp.at(n), 0);
                if(v >= myParams.minUniqNumb)
                    hMac2oneFileName.remove(macLtmp.at(n));

                if(v >= myParams.maxMacsSave){
                    //lRemovedMacs = appendThisMac2list(lRemovedMacs, s); //поки-що ігнорю, МАК будуть завжди постійними, і навряд чи будуть змінюватись
                    lRemovePaths.append(path);
                    counter++;
                }else{
                    v++;
                    hMac2counter.insert(macLtmp.at(n), v);
                }
            }

        }
    }
    return counter;
}

