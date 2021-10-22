#ifndef M2MGLOBALMETHODS_H
#define M2MGLOBALMETHODS_H

#include <QStringList>
#include <QRegularExpression>

class M2MGlobalMethods
{
public:
    static QRegularExpression getRe4thisMac(const QString &mac);

    static QRegularExpression getRe4macL(const QStringList &macL);

    static QString getPattern4thisMacL(const QStringList &macL);

    static QStringList getMacsFromFileName(int &counter, const QString &fileName);

    static QStringList appendThisMac2list(const QStringList &oldMacList, const QString &fileName);


    static QStringList getAddMacsFromFileName(int &counter, QStringList oldMacList, const QString &fileName);


};

#endif // M2MGLOBALMETHODS_H
