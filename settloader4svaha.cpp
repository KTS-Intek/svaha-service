/****************************************************************************
**
**   Copyright © 2016-2017 The KTS-INTEK Ltd.
**   Contact: http://www.kts-intek.com.ua
**
**  This file is part of svaha-service.
**
**  svaha-service is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  svaha-service is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with svaha-service.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/
#include "settloader4svaha.h"
#include "svahadefine.h"

#include <QSettings>

#include <QtCore>

//----------------------------------------------------------------------------------------------------------------------------
SettLoader4svaha::SettLoader4svaha( QObject *parent) : QObject(parent)
{    



}
//----------------------------------------------------------------------------------------------------------------------------
void SettLoader4svaha::checkDefSett()
{
    QSettings settings(path2sett(), QSettings::NativeFormat);

    qDebug() << "name " << settings.fileName() << settings.isWritable();

    QList<int> listKeys;
    listKeys << SETT_SVAHA_SERVICE_PORT << SETT_MATILDA_DEV_IP << SETT_MATILDA_CONF_IP << SETT_SVAHA_DATA_START_PORT << SETT_SVAHA_DATA_PORT_COUNT
             << SETT_ZOMBIE_MSEC << SETT_TIME_2_LIVE << SETT_ADMIN_LOGIN << SETT_ADMIN_PASSWRD << SETT_SVAHA_MAXIMUM_PENDING_CONN;
    settings.beginGroup("svaha-conf");

    for(int i = 0, iMax = listKeys.size(); i < iMax; i++){
          if(settings.contains( valName4key(listKeys.at(i)) ))
              continue;

          QVariant v;

          switch(listKeys.at(i)){
          case SETT_SVAHA_SERVICE_PORT            : v = defSETT_SVAHA_SERVICE_PORT()          ; break;
          case SETT_MATILDA_DEV_IP                : v = defSETT_MATILDA_DEV_IP()              ; break;
          case SETT_MATILDA_CONF_IP               : v = defSETT_MATILDA_CONF_IP()             ; break;
          case SETT_SVAHA_DATA_START_PORT         : v = defSETT_SVAHA_DATA_START_PORT()       ; break;
          case SETT_SVAHA_DATA_PORT_COUNT         : v = defSETT_SVAHA_DATA_PORT_COUNT()       ; break;


          case SETT_ZOMBIE_MSEC                   : v = defSETT_ZOMBIE_MSEC()                 ; break;
          case SETT_TIME_2_LIVE                   : v = defSETT_TIME_2_LIVE()                 ; break;

          case SETT_ADMIN_LOGIN                   : v = defSETT_ADMIN_LOGIN()                 ; break;
          case SETT_ADMIN_PASSWRD                 : v = defSETT_ADMIN_PASSWRD()               ; break;

          case SETT_CERBERUS_PORT                 : v = defSETT_CERBERUS_PORT()               ; break;
          case SETT_SVAHA_MAXIMUM_PENDING_CONN    : v = defSETT_SVAHA_MAXIMUM_PENDING_CONN()  ; break;

          case SETT_SYNC_WORKDIR                  : v = defSETT_SYNC_WORKDIR()                ; break;
          case SETT_SYNC_MODE                     : v = defSETT_SYNC_MODE()                   ; break;
          case SETT_SYNC_MAX_FILE_COUNT           : v = defSETT_SYNC_MAX_FILE_COUNT()         ; break;
          case SETT_SYNC_MAX_SIZE_MAC_TABLE       : v = defSETT_SYNC_MAX_SIZE_MAC_TABLE()     ; break;
          case SETT_SYNC_MAX_COUNT_SHA1_CHRSPRLL  : v = defSETT_SYNC_MAX_COUNT_SHA1_CHRSPRLL(); break;
          case SETT_SYNC_MAX_SIZE_SYNC_REQUEST    : v = defSETT_SYNC_MAX_SIZE_SYNC_REQUEST()  ; break;
          case SETT_SYNC_MAX_COUNT_SYNQ_RQSTPRLL  : v = defSETT_SYNC_MAX_COUNT_SYNQ_RQSTPRLL(); break;

          default: continue; break;
          }

          settings.setValue(  valName4key( listKeys.at(i))   , v);



          qDebug() << "checkDefSett " << listKeys.at(i) << valName4key( listKeys.at(i))  << settings.contains( valName4key(listKeys.at(i)))
                   << settings.value(valName4key(listKeys.at(i))) << v;


    }
    settings.endGroup();    

}
//----------------------------------------------------------------------------------------------------------------------------
QString SettLoader4svaha::valName4key(const int &key)
{
    QString s;
    switch(key){
    case SETT_SVAHA_SERVICE_PORT            : s = "svaha-service-port"      ; break;
    case SETT_MATILDA_DEV_IP                : s = "matilda-dev-ip"          ; break;
    case SETT_MATILDA_CONF_IP               : s = "matilda-conf-ip"         ; break;
    case SETT_SVAHA_DATA_START_PORT         : s = "start-port"              ; break;
    case SETT_SVAHA_DATA_PORT_COUNT         : s = "port-count"              ; break;


    case SETT_ZOMBIE_MSEC                   : s = "zombie-msec"             ; break;
    case SETT_TIME_2_LIVE                   : s = "time2live"               ; break;

    case SETT_ADMIN_LOGIN                   : s = "root_l"                  ; break;
    case SETT_ADMIN_PASSWRD                 : s = "root_p"                  ; break;

    case SETT_CERBERUS_PORT                 : s = "cerberus-port"           ; break;

    case SETT_SVAHA_MAXIMUM_PENDING_CONN    : s = "max-pending-conn"        ; break;

    case SETT_SYNC_WORKDIR                  : s = "sync-work-dir"           ; break;
    case SETT_SYNC_MODE                     : s = "sync-mode"               ; break;
    case SETT_SYNC_MAX_FILE_COUNT           : s = "sync-max-file-count"     ; break;
    case SETT_SYNC_MAX_SIZE_MAC_TABLE       : s = "sync-max-size-mact"      ; break;
    case SETT_SYNC_MAX_COUNT_SHA1_CHRSPRLL  : s = "sync-max-count-fsprll"   ; break;
    case SETT_SYNC_MAX_SIZE_SYNC_REQUEST    : s = "sync-max-size-reqets"    ; break;
    case SETT_SYNC_MAX_COUNT_SYNQ_RQSTPRLL  : s = "sync-max-count-rqstprll" ; break;

    case SETT_SYNC_MAX_YEAR_SAVE            : s = "sync-max-year-save"      ; break;
    case SETT_SYNC_MIN_UNIQ_MAC_FILES       : s = "sync-min-uniq-mac"       ; break;

    }
    return s;
}
//----------------------------------------------------------------------------------------------------------------------------
QVariant SettLoader4svaha::defVal4key(const int &key)
{
    QVariant v;


    switch(key){
    case SETT_SVAHA_SERVICE_PORT            : v = defSETT_SVAHA_SERVICE_PORT()          ; break;
    case SETT_MATILDA_DEV_IP                : v = defSETT_MATILDA_DEV_IP()              ; break;
    case SETT_MATILDA_CONF_IP               : v = defSETT_MATILDA_CONF_IP()             ; break;
    case SETT_SVAHA_DATA_START_PORT         : v = defSETT_SVAHA_DATA_START_PORT()       ; break;
    case SETT_SVAHA_DATA_PORT_COUNT         : v = defSETT_SVAHA_DATA_PORT_COUNT()       ; break;


    case SETT_ZOMBIE_MSEC                   : v = defSETT_ZOMBIE_MSEC()                 ; break;
    case SETT_TIME_2_LIVE                   : v = defSETT_TIME_2_LIVE()                 ; break;

    case SETT_ADMIN_LOGIN                   : v = defSETT_ADMIN_LOGIN()                 ; break;
    case SETT_ADMIN_PASSWRD                 : v = defSETT_ADMIN_PASSWRD()               ; break;

    case SETT_CERBERUS_PORT                 : v = defSETT_CERBERUS_PORT()               ; break;
    case SETT_SVAHA_MAXIMUM_PENDING_CONN    : v = defSETT_SVAHA_MAXIMUM_PENDING_CONN()  ; break;

    case SETT_SYNC_WORKDIR                  : v = defSETT_SYNC_WORKDIR()                ; break;
    case SETT_SYNC_MODE                     : v = defSETT_SYNC_MODE()                   ; break;
    case SETT_SYNC_MAX_FILE_COUNT           : v = defSETT_SYNC_MAX_FILE_COUNT()         ; break;
    case SETT_SYNC_MAX_SIZE_MAC_TABLE       : v = defSETT_SYNC_MAX_SIZE_MAC_TABLE()     ; break;
    case SETT_SYNC_MAX_COUNT_SHA1_CHRSPRLL  : v = defSETT_SYNC_MAX_COUNT_SHA1_CHRSPRLL(); break;
    case SETT_SYNC_MAX_SIZE_SYNC_REQUEST    : v = defSETT_SYNC_MAX_SIZE_SYNC_REQUEST()  ; break;
    case SETT_SYNC_MAX_COUNT_SYNQ_RQSTPRLL  : v = defSETT_SYNC_MAX_COUNT_SYNQ_RQSTPRLL(); break;

    case SETT_SYNC_MAX_YEAR_SAVE            : v = defSETT_SYNC_MAX_YEAR_SAVE()          ; break;
    case SETT_SYNC_MIN_UNIQ_MAC_FILES       : v = defSETT_SYNC_MIN_UNIQ_MAC_FILES()     ; break;
    }
    return v;
}
//----------------------------------------------------------------------------------------------------------------------------
QVariant SettLoader4svaha::loadOneSett(const int key)
{

    QSettings settings(path2sett(), QSettings::NativeFormat);
    if(settings.fileName().isEmpty()){
        qDebug() << "sett is empty " << settings.fileName();
        return false;
    }



    QString valKey = valName4key(key);
    if(valKey.isEmpty()){
        qDebug() << "loadOneSett unknown key " << key ;
        return defVal4key(key);
    }

    settings.beginGroup("svaha-conf");
    QVariant v = settings.value(valKey, defVal4key(key));
    settings.endGroup();

    return v;
}
//----------------------------------------------------------------------------------------------------------------------------
bool SettLoader4svaha::saveOneSett(const int key, const QVariant data2save)
{

    QSettings settings(path2sett(), QSettings::NativeFormat);
    if(settings.fileName().isEmpty()){
        qDebug() << "sett is empty " << settings.fileName();
        return false;
    }
    QString valKey = valName4key(key);

    if(valKey.isEmpty())
        return false;

    settings.beginGroup("svaha-conf");

    settings.setValue(valKey, data2save);
    settings.endGroup();

    return true;

}
//--------------------------------------------------------------------------------------------------------
QString SettLoader4svaha::path2sett()
{
//    return QString("%1/svaha.conf").arg(qApp->applicationDirPath());
    QDir dir(qApp->applicationDirPath());
    QString path2sett ;

    if(dir.cdUp()){
        dir.setPath(QString("%1/config").arg(dir.path()));
        path2sett = dir.path();
        if(!dir.exists())
            dir.mkpath(path2sett);

        path2sett.append(QString("/svaha"));

    }

    if(path2sett.isEmpty())
        path2sett = "/dev/null";

    return path2sett;
}

//--------------------------------------------------------------------------------------------------------
QVariantHash SettLoader4svaha::loadSettByKey(const QStringList &lk, const QList<int> &lks)
{
    QVariantHash h;

    QSettings settings(path2sett(), QSettings::NativeFormat);
    if(settings.fileName().isEmpty()){
        qDebug() << "sett is empty " << settings.fileName();
        return h;
    }
    settings.beginGroup("svaha-conf");

    for(int i = 0, iMax = lk.size(); i < iMax; i++){
        int key = lks.at(i);
        QString valKey = valName4key(key);
        if(valKey.isEmpty()){
            qDebug() << "loadSettByKey unknown key " << key ;
            h.insert(lk.at(i), defVal4key(key));
            continue;
        }
        h.insert(lk.at(i), settings.value(valKey, defVal4key(key)));
    }
    settings.endGroup();
    return h;
}

//--------------------------------------------------------------------------------------------------------

bool SettLoader4svaha::saveSettByKey(const QVariantHash &h, const QStringList &lk, const QList<int> &lks)
{
    bool r = false;
    QSettings settings(path2sett(), QSettings::NativeFormat);
    if(settings.fileName().isEmpty()){
        qDebug() << "sett is empty " << settings.fileName();
        return r;
    }
    settings.beginGroup("svaha-conf");

    for(int i = 0, iMax = lk.size(); i < iMax; i++){
        if(!h.contains(lk.at(i)))
            continue;
        QString valKey = valName4key(lks.at(i));
        if(valKey.isEmpty()){
            qDebug() << "saveSettByKey unknown key " << lks.at(i) ;
            continue;
        }
        r = true;
        settings.setValue(valKey, h.value(lk.at(i)));
    }
    settings.endGroup();
    return r;
}

//--------------------------------------------------------------------------------------------------------

quint16 SettLoader4svaha::defSETT_SVAHA_SERVICE_PORT()        { return (quint16)65000   ;}

QString SettLoader4svaha::defSETT_MATILDA_DEV_IP()            { return "svaha.ddns.net" ;}

QString SettLoader4svaha::defSETT_MATILDA_CONF_IP()           { return "svaha.ddns.net" ;}

quint16 SettLoader4svaha::defSETT_SVAHA_DATA_START_PORT()     { return (quint16)50000   ;}


quint16 SettLoader4svaha::defSETT_SVAHA_DATA_PORT_COUNT()     { return (quint16)100      ; } //out server count


int SettLoader4svaha::defSETT_ZOMBIE_MSEC()                   { return (15 * 60 * 1000)     ; }

int SettLoader4svaha::defSETT_TIME_2_LIVE()                   { return (24 * 60 * 60 * 1000) ; }

QString SettLoader4svaha::defSETT_ADMIN_LOGIN()               { return "root";         } // QCryptographicHash::hash(QByteArray("root"), QCryptographicHash::Sha3_256);

QString SettLoader4svaha::defSETT_ADMIN_PASSWRD()             { return "ChystaKrynytsa-Trutni-W.H.I.T.E"; } //QCryptographicHash::hash(QByteArray(""), QCryptographicHash::Sha3_256);

quint16 SettLoader4svaha::defSETT_CERBERUS_PORT()             { return (quint16)50000; } //після налаштування роутера замінити на 65001

quint16 SettLoader4svaha::defSETT_SVAHA_MAXIMUM_PENDING_CONN(){ return (quint16)500  ; }


QString SettLoader4svaha::defSETT_SYNC_WORKDIR()
{
    QDir dir(qApp->applicationDirPath());
    dir.cdUp();

    QString s = dir.absolutePath() + "/backups";
    dir.setPath(s);
    if(!dir.exists())
        dir.mkpath(s);
    return s;
}


quint8 SettLoader4svaha::defSETT_SYNC_MODE()                    { return DT_MODE_EVERY_DAY  ; }

quint32 SettLoader4svaha::defSETT_SYNC_MAX_FILE_COUNT()         { return 10                 ; }

quint32 SettLoader4svaha::defSETT_SYNC_MAX_SIZE_MAC_TABLE()     { return 10000              ; }

quint32 SettLoader4svaha::defSETT_SYNC_MAX_COUNT_SHA1_CHRSPRLL(){ return 10                 ; }

quint32 SettLoader4svaha::defSETT_SYNC_MAX_SIZE_SYNC_REQUEST()  { return 10000              ; }

quint32 SettLoader4svaha::defSETT_SYNC_MAX_COUNT_SYNQ_RQSTPRLL(){ return 10                 ; }

qint32 SettLoader4svaha::defSETT_SYNC_MAX_YEAR_SAVE()           { return 0                  ; }//no limit

qint32 SettLoader4svaha::defSETT_SYNC_MIN_UNIQ_MAC_FILES()      { return 3                  ; }

QString SettLoader4svaha::strFromStrHash(const QStringHash &h)
{
    QList<QString> lk = h.keys();
    QStringList l;
    for(int i = 0, iMax = lk.size(); i < iMax; i++)
        l.append(QString("%1=%2").arg(lk.at(i)).arg(h.value(lk.at(i))));
    return l.join("; ");
}


