#include "oldbackupcleaner.h"
#include <QtCore>
#include "src/matilda/matildaprotocolhelper.h"
#include "svahadefine.h"

//--------------------------------------------------------------------------------------------------
OldBackupCleaner::OldBackupCleaner(const qint32 &maxYearSave, const qint32 &maxMacsSave, const qint32 &minUniqNumb, const QString &workDir, QObject *parent) : QObject(parent)
{
    setMaxSett(maxYearSave, maxMacsSave, minUniqNumb, workDir);
}
//--------------------------------------------------------------------------------------------------
void OldBackupCleaner::onThreadStarted()
{
    clearOldFileCounter = 0;
    QTimer *t = new QTimer(this);
    t->setSingleShot(true);
    connect(this, SIGNAL(startTmrCheckMacs(int)), t, SLOT(start(int)) );
    connect(t, SIGNAL(timeout()), this, SLOT(onCheckMacsTmr()) );

    emit startTmrCheckMacs(999);
}
//--------------------------------------------------------------------------------------------------
void OldBackupCleaner::setMaxSett(qint32 maxYearSave, qint32 maxMacsSave, qint32 minUniqNumb, QString workDir)
{
    if(maxMacsSave < 0)
        maxMacsSave = 0;
    this->maxMacsSave = maxMacsSave;
    this->maxYearSave = maxYearSave;
    this->workDir = workDir;
    if(minUniqNumb > maxMacsSave)
        minUniqNumb = maxMacsSave;
    if(minUniqNumb < 1)
        minUniqNumb = 1;
    this->minUniqNumb = minUniqNumb;

}
//--------------------------------------------------------------------------------------------------
void OldBackupCleaner::onFileCreated(QStringList macL)
{
    checkRemovedMacs(macL, macL.size());
}
//--------------------------------------------------------------------------------------------------
void OldBackupCleaner::checkRemovedMacs(QStringList macL, int counter)
{
    for(int i = 0; i < counter; i++){
        if(hMacQueue.contains(macL.at(i)))
            continue;
        listMacQueue.append(macL.at(i));
        hMacQueue.insert(macL.at(i), true);
    }
}
//--------------------------------------------------------------------------------------------------
void OldBackupCleaner::onCheckMacsTmr()
{    
    if(maxYearSave > 0)
        clearOldFile();
    if(maxMacsSave > 0)
        clearFileByMac();

    emit startTmrCheckMacs( listMacQueue.isEmpty() ? 555 : 11);
}
//--------------------------------------------------------------------------------------------------
void OldBackupCleaner::clearOldFile()
{
    //<work dir>/<year>/<month>/<file names>  //UTC date time!!!
    //file name <base sha1>_<mac(X)>_...other keys

    if(clearOldFileCounter++ != 0)//щоб не так часто
        return;

    QDir dir(workDir);
    if(!dir.exists())
        return;


    QDate dt = QDateTime::currentDateTimeUtc().date();
    dt.setDate(dt.year(), dt.month(), 1);
    dt = dt.addYears((-1) * maxYearSave);
    QDate dtMin = QDateTime(QDate(2017, 1, 1), QTime(0,0,0,0), Qt::UTC).addYears(-10).date();

    for(int j = 0; dt > dtMin && j < 10000; j++){
        QString lastPath = QString("%1/%2/%3").arg(workDir).arg(dt.year()).arg(dt.month());
        dir.setPath(lastPath);
        if(!dir.exists())
            continue;
        bool r = dir.removeRecursively();
        qDebug() << "remove dir " << r << dir.path() ;
        dt = dt.addMonths(-1);
    }
}
//--------------------------------------------------------------------------------------------------
void OldBackupCleaner::clearFileByMac()
{
    //<work dir>/<year>/<month>/<file names>  //UTC date time!!!
    //file name <base64 ( omit "=",  replace("/", "=")) sha1>_<mac(X)>_...other keys

    if(listMacQueue.isEmpty())
        return;

    QDir dir(workDir);
    if(!dir.exists())
        return;

    QStringList macL;
    for(int i = 0; i < 30 && !listMacQueue.isEmpty(); i++){
        hMacQueue.remove(listMacQueue.first());
        macL.append(listMacQueue.takeFirst());
    }

    QRegularExpression re("*");
    if(true){
        QString pattern;
        QStringList patternl;
        for(int i = 0, iMax = macL.size(); i < iMax; i++)
            patternl.append(QString("(?=.*MAC[\\d]:%1_)").arg(macL.at(i)));
        pattern = patternl.join("|");
        pattern.prepend("^");
        pattern.append(".*$");
        re.setPattern(pattern);
    }

    QDate dt = QDateTime::currentDateTimeUtc().date();
    dt.setDate(dt.year(), dt.month(), 1);
    QDate dtMin = dt.addYears((-1) * maxYearSave);// QDateTime(QDate(2017, 1, 1), QTime(0,0,0,0), Qt::UTC).date();

    QHash<QString,qint32> hMac2counter;
    //всовую пару МАК назва файлу, якщо в МАК повторюється видаляю пару, тобто якщо буде лише один файл з МАКом,
    //то він залишиться в контейнері, при видаленні файлів буде заборона на видалення файлів з цього контейнера
    QHash<QString,QString> hMac2oneFileName;

    QStringList lRemovePaths;
    QStringList lEmptyDirectory;

    int counter = 0;

    for(int j = 0, nMax = macL.size(); dt > dtMin && j < 10000; j++){

        QString lastPath = QString("%1/%2/%3").arg(workDir).arg(dt.year()).arg(dt.month());

        dir.setPath(lastPath);
        dt = dt.addMonths(-1);

        QStringList fileNames = dir.entryList(QDir::Files|QDir::NoDotAndDotDot|QDir::NoSymLinks|QDir::Readable, QDir::Time);
        if(fileNames.isEmpty()){
            if(dir.exists())
                lEmptyDirectory.append(lastPath);
            continue;
        }


        for(int i = 0, iMax = fileNames.size(); i < iMax && i < 1000000; i++){
            QString s = fileNames.at(i);
            if(s.contains(re)){
                //found matches
                QString path = lastPath + "/" + s;
                QStringList macLtmp = getMacsFromFileName(nMax, s);

                for(int n = 0; n < nMax && n < 100; n++){
                    if(!hMac2counter.contains(macLtmp.at(n))){
                        //first
                        hMac2oneFileName.insert(macLtmp.at(n), path);
                        hMac2counter.insert(macLtmp.at(n), 1);
                        continue;
                    }

                    qint32 v = hMac2counter.value(macLtmp.at(n), 0);
                    if(v >= minUniqNumb)
                        hMac2oneFileName.remove(macLtmp.at(n));

                    if(v >= maxMacsSave){
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

    }

    QList<QString> lk = hMac2oneFileName.keys();
    QHash<QString,bool> hIgnoreFileNames;
    for(int i = 0, iMax = lk.size(); i < iMax; i++)
        hIgnoreFileNames.insert(hMac2oneFileName.value(lk.at(i)), true);

    hMac2oneFileName.clear();
    lk.clear();
    hMac2counter.clear();

    for(int i = 0, iMax = counter; i < iMax; i++){
        if(hIgnoreFileNames.value(lRemovePaths.at(i), false))
            continue;

        bool r = dir.remove(lRemovePaths.at(i));
        qDebug() << "remove file " << r << lRemovePaths.at(i);

    }

    while(!lEmptyDirectory.isEmpty()){
        dir.setPath(lEmptyDirectory.takeFirst());
        bool r = dir.removeRecursively();
        qDebug() << "remove dir0 " << r << dir.path() ;
    }

}
//--------------------------------------------------------------------------------------------------
QStringList OldBackupCleaner::getMacsFromFileName(int &counter, const QString &fileName)
{
    QStringList macL;
    counter = 0;
    QStringList l = fileName.split("MAC", QString::SkipEmptyParts);
    for(int i = 1, iMax = l.size(); i < iMax; i++){
        QString s = l.at(i);
        if(s.right(1) == "_"){
            s.chop(1);
            s = s.mid(2);
            if(!macL.contains(s)){
                macL.append(s);
                counter++;
            }
        }else{
            if(s.contains("_")){
                s = s.left(s.indexOf("_"));
                s = s.mid(2);
                if(!macL.contains(s)){
                    macL.append(s);
                    counter++;
                }
            }
            break;
        }
    }
    return macL;
}
//--------------------------------------------------------------------------------------------------
