#include "checklocalfilesha1.h"
#include <QtCore>
#include "matildaprotocolhelper.h"

//--------------------------------------------------------------------------------
CheckLocalFileSha1::CheckLocalFileSha1(const LclFileSett &fsett, QObject *parent) : QObject(parent)
{
    this->fsett = fsett;
}

//--------------------------------------------------------------------------------

void CheckLocalFileSha1::onThreadStarted()
{
    QTime t;
    t.start();
    startCheck();
    qDebug() << "fs check time= " << t.elapsed();
    deleteLater();
}
//--------------------------------------------------------------------------------
void CheckLocalFileSha1::startCheck()
{
    //<work dir>/<year>/<month>/<file names>  //UTC date time!!!

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
    QList<QByteArray> lfoundSha1;
    QList<QDateTime> dtCreatedUtcL;
    int counter = 0;

    for(int j = 0, nMax = fsett.macL.size(); dt > dtMin && j < 10000; j++){

        QString lastPath = QString("%1/%2").arg(dt.year()).arg(dt.month());
        dir.setPath(fsett.workDir + "/" + lastPath);

        QStringList fileNames = dir.entryList(QDir::Files|QDir::NoDotAndDotDot|QDir::NoSymLinks|QDir::Readable, QDir::Time|QDir::Reversed);

//        QFileInfoList fileNames = dir.entryInfoList(QStringList(), QDir::Files|QDir::NoDotAndDotDot|QDir::NoSymLinks|QDir::Readable);

        for(int i = 0, iMax = fileNames.size(); i < iMax && i < 1000000; i++){
            QString s = fileNames.at(i);
            if(s.contains(re)){
                //found matches
                for(int n = 0; n < nMax && n < 100; n++){
                    if(s.contains(QString(QRegularExpression(QString("^(?=.*MAC[\\d]:%1_).*$").arg(fsett.macL.at(n)))))){
                        if(!hMac2counter.contains(fsett.macL.at(n))){
                            //first, calculate sha1
                            bool ok;
                            QByteArray sha1 = MatildaProtocolHelper::calcFileSha1(fsett.workDir + "/" + lastPath + "/" + s, ok);
                            if(ok){
                               lfoundMac.append(fsett.macL.at(n));
                               lfoundSha1.append(sha1);
                               counter++;
                               hMac2counter.insert(fsett.macL.at(n), 1);

                               QFileInfo fi(fsett.workDir + "/" + lastPath + "/" + s);
                               dtCreatedUtcL.append(fi.created().toUTC());
                            }else{
                                qDebug() << "can't calculate sha1 " << sha1 << ok << lastPath << s;
                            }
                            continue;
                        }

                        int v = hMac2counter.value(fsett.macL.at(n), 0);
                        if(v > fsett.maxFileCount){
                            dir.remove(fsett.workDir + "/" + lastPath + "/" + s);
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
        case DT_MODE_EVERY_DAY  : dt = dt.addDays(-1); break;
        case DT_MODE_EVERY_WEEK : dt = dt.addDays(-7); break;
        case DT_MODE_EVERY_YEAR : dt = dt.addMonths(-1); break;
        default: return;
        }
    }



    QStringList macs = fsett.macL;

    if(!lfoundMac.isEmpty()){
        for(int i = 0; i < counter; i++){
            if(macs.contains(lfoundMac.at(i)))
                macs.removeAll(lfoundMac.at(i));
        }

        emit setLocalMacDateSha1(lfoundMac, lfoundSha1, dtCreatedUtcL, counter);
    }

    if(!macs.isEmpty())
        emit appendMac2queue(macs, macs.size());



}
//--------------------------------------------------------------------------------
