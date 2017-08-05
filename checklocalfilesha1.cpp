#include "checklocalfilesha1.h"
#include <QtCore>
#include "matildaprotocolhelper.h"

//--------------------------------------------------------------------------------
CheckLocalFileSha1::CheckLocalFileSha1(const QStringList &macL, const QString &workDir, const quint8 &dtMode, const quint8 &maxFileCount, QObject *parent) : QObject(parent)
{
    this->fsett.macL = macL;
    this->fsett.workDir = workDir;
    this->fsett.dtMode = dtMode;
    this->fsett.maxFileCount = maxFileCount;
}

//--------------------------------------------------------------------------------

void CheckLocalFileSha1::onThreadStarted()
{
    QTime t;
    t.start();
    startCheck();
    qDebug() << "fs check time= " << t.elapsed() << fsett.macL.join(" ");
    deleteLater();
}
//--------------------------------------------------------------------------------
void CheckLocalFileSha1::startCheck()
{
    //<work dir>/<year>/<month>/<file names>  //UTC date time!!!
    //file name <base64 sha1>_<mac(X)>_...other keys

    QDir dir(fsett.workDir);
    if(!dir.exists())
        return;

    QRegularExpression re("*");
    if(true){
        QString pattern;
        QStringList patternl;
        for(int i = 0, iMax = fsett.macL.size(); i < iMax; i++)
            patternl.append(QString("(?=.*MAC[\\d]:%1_)").arg(fsett.macL.at(i).toLower()));
        pattern = patternl.join("|");
        pattern.prepend("^");
        pattern.append(".*$");
        re.setPattern(pattern);
    }

    QDate dt = QDateTime::currentDateTimeUtc().date();
    QDate dtMin = QDateTime(QDate(2017, 1, 1), QTime(0,0,0,0), Qt::UTC).date();

    QHash<QString,int> hMac2counter;
    QStringList lfoundMac;
    QStringList lfoundSha1;
    QList<QDateTime> dtCreatedUtcL;
    int counter = 0;

    QStringList lRemovedMacs;

    for(int j = 0, nMax = fsett.macL.size(); dt > dtMin && j < 10000; j++){

        QString lastPath = QString("%1/%2/%3").arg(fsett.workDir).arg(dt.year()).arg(dt.month());
        dir.setPath(lastPath);

        QStringList fileNames = dir.entryList(QDir::Files|QDir::NoDotAndDotDot|QDir::NoSymLinks|QDir::Readable, QDir::Time|QDir::Reversed);
        for(int i = 0, iMax = fileNames.size(); i < iMax && i < 1000000; i++){
            QString s = fileNames.at(i);
            if(s.contains(re)){
                //found matches
                for(int n = 0; n < nMax && n < 100; n++){

                    if(s.contains(QString(QRegularExpression(QString("^(?=.*_MAC[\\d]:%1_).*$").arg(fsett.macL.at(n)))))){ //<base 64 sha1>_MAC0:12:23:23:ab:ac

                        if(!hMac2counter.contains(fsett.macL.at(n))){
                            //first

                            QStringList sha1 = s.split("_").first();// MatildaProtocolHelper::calcFileSha1(fsett.workDir + "/" + lastPath + "/" + s, ok);
                            if(!s.isEmpty()){
                               lfoundMac.append(fsett.macL.at(n));
                               lfoundSha1.append(sha1);
                               counter++;
                               hMac2counter.insert(fsett.macL.at(n), 1);

                               dtCreatedUtcL.append(QFileInfo(lastPath + "/" + s).created().toUTC());
                            }else{
                                qDebug() << "can't calculate sha1 " << sha1 << lastPath << s;
                            }
                            continue;
                        }

                        int v = hMac2counter.value(fsett.macL.at(n), 0);
                        if(v > fsett.maxFileCount){
                            //lRemovedMacs = appendThisMac2list(lRemovedMacs, s); //поки-що ігнорю, МАК будуть завжди постійними, і навряд чи будуть змінюватись
                            dir.remove(lastPath + "/" + s);
                        }else{
                            v++;
                            hMac2counter.insert(fsett.macL.at(n), v);
                        }
                    }
                }
                continue;
            }
        }

        switch(fsett.dtMode){
        case DT_MODE_EVERY_DAY  : dt = dt.addDays(-1)  ; break;
        case DT_MODE_EVERY_WEEK : dt = dt.addDays(-7)  ; break;
        case DT_MODE_EVERY_MONTH: dt = dt.addMonths(-1); break;
        default: dt = dtMin.addYears(-11); break;
        }
    }

    //Всі МАК адреси додаються до однієї таблиці, але з різними мітками дати, кому відправляти запит на синхронізацію вирішувати не тут
    QStringList macs = fsett.macL;
    if(!lfoundMac.isEmpty()){
        for(int i = 0; i < counter; i++){
            if(macs.contains(lfoundMac.at(i)))
                macs.removeAll(lfoundMac.at(i));
        }
        //МАК адреси що мають дані по синхронізації за обраний період
        emit setLocalMacDateSha1(lfoundMac, lfoundSha1, dtCreatedUtcL, counter);
    }

    if(!macs.isEmpty())//МАК адреси що не мають даних по синхронізації за обраний період
        emit appendMac2queueSyncRequest(macs, macs.size());



//    if(!lRemovedMacs.isEmpty()) //якщо видаляю по старості, то значить мають буди якісь дані
//        emit hasRemovedMacs(lRemovedMacs, counter);

}
//--------------------------------------------------------------------------------
QStringList CheckLocalFileSha1::appendThisMac2list(QStringList oldMacList, const QString &fileName)
{
    //<MACx:90:90:90>_
    //MAC0:01:02:03:04:05:06:07:08_MAC1:10:20:30:40:50:60:70:80_ID:LALA
    QStringList l = fileName.split("MAC:", QString::SkipEmptyParts);
    for(int i = 0, iMax = l.size(); i < iMax; i++){
        QString s = l.at(i);
        if(s.right(1) == "_"){
            s.chop(1);
            if(!oldMacList.contains(s))
                oldMacList.append(s);
        }else{
            if(s.contains("_")){
                s = s.left(s.indexOf("_"));
                if(!oldMacList.contains(s))
                    oldMacList.append(s);
            }
            break;
        }
    }
    return oldMacList;
}
//--------------------------------------------------------------------------------
