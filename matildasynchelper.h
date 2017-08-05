#ifndef MATILDASYNCHELPER_H
#define MATILDASYNCHELPER_H

#include <QObject>
#include <QtCore>

class MatildaSyncHelper : public QObject
{
    Q_OBJECT
public:
    explicit MatildaSyncHelper(QObject *parent = 0);

    static QByteArray calcSHA1_4File(const QString &fileName, bool &ok);


signals:

public slots:
};

#endif // MATILDASYNCHELPER_H
