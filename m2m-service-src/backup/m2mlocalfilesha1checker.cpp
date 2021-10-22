#include "m2mlocalfilesha1checker.h"

#include <QDir>
#include <QDebug>


///[!] svaha-service
#include "m2m-service-src/main/m2mglobalmethods.h"







M2MLocalFileSha1Checker::M2MLocalFileSha1Checker(const bool &verboseMode, QObject *parent) : QObject(parent)
{
    fsett.verboseMode = verboseMode;
}

void M2MLocalFileSha1Checker::setSettings(const QStringList &macL, const QString &workDir, const quint8 &dtMode, const quint8 &maxFileCount)
{
    fsett.macL = macL;
    fsett.workDir = workDir;
    fsett.dtMode = dtMode;
    fsett.maxFileCount = maxFileCount;
}

int M2MLocalFileSha1Checker::findTheseMacs(const QStringList &fileNames, const QString &lastPath, const int &nMax, const QRegularExpression &re, const QStringList &macL,
                                           QHash<QString, qint32> &hMac2counter, QStringList &lfoundMac, QStringList &lfoundSha1,
                                           QList<qint64> &msecCreatedL, int &counterCheckRemoveMacs, QStringList &lCheckMac4remove)
{
    int counter = 0;
    for(int i = 0, iMax = fileNames.size(); i < iMax && i < 1000000; i++){
        const QString s = fileNames.at(i);
        if(s.contains(re)){
            //found matches
            for(int n = 0; n < nMax && n < 100; n++){

                const QString mac = macL.at(n);

                if(s.contains(M2MGlobalMethods::getRe4thisMac(mac))){
                    // QRegularExpression(QString("^(?=.*_MAC[\\d]:%1_).*$").arg(mac)))){ //<base 64 sha1>_MAC0:12:23:23:ab:ac

                    if(!hMac2counter.contains(mac)){
                        //first

                        QString sha1 = s.split("_").first();// MatildaProtocolHelper::calcFileSha1(fsett.workDir + "/" + lastPath + "/" + s, ok);
                        sha1 = sha1.replace("=", "/");//заборонений у ФС символ / замінений на =
                        if(!s.isEmpty()){
                           lfoundMac.append(mac);
                           lfoundSha1.append(sha1);
                           counter++;
                           hMac2counter.insert(mac, 1);

                           msecCreatedL.append(QFileInfo(lastPath + "/" + s).birthTime().toMSecsSinceEpoch());
                           continue;
                        }
                        if(fsett.verboseMode)
                            qDebug() << "can't calculate sha1 " << mac << sha1 << lastPath << s;

                        continue;
                    }

                    int v = hMac2counter.value(mac, 0);
                    if(v >= fsett.maxFileCount){
                        if(!lCheckMac4remove.contains(mac)){
                            lCheckMac4remove.append(mac);
                            counterCheckRemoveMacs++;
                        }
                    }else{
                        v++;
                        hMac2counter.insert(mac, v);
                    }
                }
            }
            continue;
        }
    }

    return counter;
}

void M2MLocalFileSha1Checker::onThreadStarted()
{
    QTime t;
    t.start();
//    fsett.macL = fsett.macL.join("\n").toLower().split("\n");
    startCheck();

    if(fsett.verboseMode)
        qDebug() << "fs check time= " << t.elapsed() << fsett.macL.join(" ");
    deleteLater();
}

void M2MLocalFileSha1Checker::startCheck()
{
    if(fsett.workDir.isEmpty() || fsett.macL.isEmpty())
        return;

    QDir dir(fsett.workDir);
    if(!dir.exists())
        return;

    const QRegularExpression re = M2MGlobalMethods::getRe4macL(fsett.macL);
    QDate dt = QDateTime::currentDateTimeUtc().date();
    dt.setDate(dt.year(), dt.month(), 1);
    QDate dtMin = QDateTime(QDate(2017, 1, 1), QTime(0,0,0,0), Qt::UTC).date();

    QHash<QString,qint32> hMac2counter;
    QStringList lfoundMac;
    QStringList lfoundSha1;
    QList<qint64> msecCreatedL;
    int counter = 0;
    QStringList lCheckMac4remove;
    int counterCheckRemoveMacs = 0;

    for(int j = 0, nMax = fsett.macL.size(); dt > dtMin && j < 10000; j++){

        const QString lastPath = QString("%1/%2/%3").arg(fsett.workDir).arg(dt.year()).arg(dt.month());
        dir.setPath(lastPath);

        const QStringList fileNames = dir.entryList(QDir::Files|QDir::NoDotAndDotDot|QDir::NoSymLinks|QDir::Readable, QDir::Time);

        counter += findTheseMacs(fileNames, lastPath, nMax, re, fsett.macL,
                                 hMac2counter, lfoundMac, lfoundSha1,
                                 msecCreatedL, counterCheckRemoveMacs, lCheckMac4remove);

        dt = dt.addMonths(-1);


    }

    //Всі МАК адреси додаються до однієї таблиці, але з різними мітками дати, кому відправляти запит на синхронізацію вирішувати не тут
    QStringList macs = fsett.macL;
    if(!lfoundMac.isEmpty()){
        for(int i = 0; i < counter; i++){
            if(macs.contains(lfoundMac.at(i)))
                macs.removeAll(lfoundMac.at(i));
        }
        //МАК адреси що мають дані по синхронізації за обраний період
        emit setLocalMacDateSha1(lfoundMac, lfoundSha1, msecCreatedL, counter);
    }

    if(!macs.isEmpty())//МАК адреси що не мають даних по синхронізації за обраний період
        emit appendMac2queueSyncRequest(macs, macs.size());

    if(counterCheckRemoveMacs > 0)
        emit checkRemovedMacs(lCheckMac4remove, counterCheckRemoveMacs);
//M2MGlobalMethods::appendThisMac2list(const QStringList &oldMacList, const QString &fileName)

    //    if(!lRemovedMacs.isEmpty()) //якщо видаляю по старості, то значить мають буди якісь дані
    //        emit hasRemovedMacs(lRemovedMacs, counter);
}

