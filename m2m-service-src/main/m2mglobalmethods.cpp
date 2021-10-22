#include "m2mglobalmethods.h"


QRegularExpression M2MGlobalMethods::getRe4thisMac(const QString &mac)
{
    return QRegularExpression(QString("^(?=.*_MAC[\\d]:%1_).*$").arg(mac));
}

QRegularExpression M2MGlobalMethods::getRe4macL(const QStringList &macL)
{
    return QRegularExpression(getPattern4thisMacL(macL));
}

QString M2MGlobalMethods::getPattern4thisMacL(const QStringList &macL)
{
    //src backupskiller
    QString pattern;
    QStringList patternl;
    for(int i = 0, iMax = macL.size(); i < iMax; i++)
        patternl.append(QString("(?=.*MAC[\\d]:%1_)").arg(macL.at(i)));
    pattern = patternl.join("|");
    pattern.prepend("^");
    pattern.append(".*$");
    return pattern;
}

QStringList M2MGlobalMethods::getMacsFromFileName(int &counter, const QString &fileName)
{
    //src backupskiller
    counter = 0;

    return getAddMacsFromFileName(counter, QStringList(), fileName);

}

QStringList M2MGlobalMethods::appendThisMac2list(const QStringList &oldMacList, const QString &fileName)
{
    //src CheckLocalFileSha1
    int counter = 0;
    return getAddMacsFromFileName(counter, oldMacList, fileName);
}

QStringList M2MGlobalMethods::getAddMacsFromFileName(int &counter, QStringList oldMacList, const QString &fileName)
{
    //<MACx:90:90:90>_
    //*_MAC0:01:02:03:04:05:06:07:08_MAC1:10:20:30:40:50:60:70:80_ID:LALA

    const QStringList l = fileName.split("MAC", QString::SkipEmptyParts);

    for(int i = 1, iMax = l.size(); i < iMax; i++){
        QString s = l.at(i);
        if(s.right(1) == "_"){
            s.chop(1);
            s = s.mid(2);
            if(!oldMacList.contains(s)){
                oldMacList.append(s);
                counter++;
            }
        }else{
            if(s.contains("_")){
                s = s.left(s.indexOf("_"));
                s = s.mid(2);
                if(!oldMacList.contains(s)){
                    oldMacList.append(s);
                    counter++;
                }
            }
            break;
        }
    }
    return oldMacList;
}
