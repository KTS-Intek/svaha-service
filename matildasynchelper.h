#ifndef MATILDASYNCHELPER_H
#define MATILDASYNCHELPER_H

#include <QObject>

class MatildaSyncHelper : public QObject
{
    Q_OBJECT
public:
    explicit MatildaSyncHelper(QObject *parent = 0);

signals:

public slots:
};

#endif // MATILDASYNCHELPER_H