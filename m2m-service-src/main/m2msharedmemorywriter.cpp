#include "m2msharedmemorywriter.h"

///[!] type-converter
#include "src/base/convertatype.h"



//-------------------------------------------------------------------------------------

M2MSharedMemoryWriter::M2MSharedMemoryWriter(const QString &sharedMemoName, const QString &semaName, const bool &verboseMode, QObject *parent)
    : SharedMemoWriter(sharedMemoName, semaName, "", 1111, 66666, verboseMode,  parent)
{

}

//-------------------------------------------------------------------------------------

QString M2MSharedMemoryWriter::strFromStrHash(const QStringHash &h)
{

//    const auto oneObjInfo = hashAboutObject.value(mac);

    return ConvertAtype::qslFromHash(h, "=", h.keys()).join("; ");


//    QList<QString> lk = h.keys();
//    QStringList l;
//    for(int i = 0, iMax = lk.size(); i < iMax; i++)
//        l.append(QString("%1=%2").arg(lk.at(i)).arg(h.value(lk.at(i))));
//    return l.join("; ");
}

//-------------------------------------------------------------------------------------

QVariantHash M2MSharedMemoryWriter::fromConnectionTable(QStringHash hashMacRemoteId, QStringHash hashMacDevId, QStringHash hashTime, QStringHashHash hashAboutObject)
{
    QVariantHash out;
    auto lk = hashMacRemoteId.keys();
    for(int i = 0, iMax = lk.size(); i < iMax; i++){
        const QString mac = lk.at(i);
        out.insert( mac, QStringList() << hashMacRemoteId.value(mac) << hashMacDevId.value(mac) << hashTime.value(mac)
                  << strFromStrHash(hashAboutObject.value(mac)) );
    }
    return out;
}

//-------------------------------------------------------------------------------------

void M2MSharedMemoryWriter::onThreadStarted()
{
    mymaximums.write2ram = 60;


    setMirrorMode(true);
    initObjectLtr();


}

//-------------------------------------------------------------------------------------

void M2MSharedMemoryWriter::onConnectionTableChanged()
{
    changeSharedMemArrDataCounter();
}

//-------------------------------------------------------------------------------------

void M2MSharedMemoryWriter::onConnectionTableData(QStringHash hashMacRemoteId, QStringHash hashMacDevId, QStringHash hashTime, QStringHashHash hashAboutObject)
{
    QVariantHash h;
    h.insert("crbr", fromConnectionTable(hashMacRemoteId, hashMacDevId, hashTime, hashAboutObject));

    flushNowArr(SharedMemoHelper::getArrFromVarHash(h));


}

//-------------------------------------------------------------------------------------
