/****************************************************************************
**
**   Copyright © 2016-2017 The KTS-INTEK Ltd.
**   Contact: http://www.kts-intek.com.ua
**
**  This file is part of svaha-service-kts.
**
**  svaha-service-kts is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  svaha-service-kts is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with svaha-service-kts.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/
#include "settloader4svaha.h"
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
             << SETT_ZOMBIE_MSEC << SETT_TIME_2_LIVE << SETT_ADMIN_LOGIN << SETT_ADMIN_PASSWRD;
    settings.beginGroup("svaha-conf");

    for(int i = 0, iMax = listKeys.size(); i < iMax; i++){
          if(settings.contains( valName4key(listKeys.at(i)) ))
              continue;

          QVariant var;

          switch(listKeys.at(i)){
          case SETT_SVAHA_SERVICE_PORT : var = defSETT_SVAHA_SERVICE_PORT(); break;
          case SETT_MATILDA_DEV_IP : var = defSETT_MATILDA_DEV_IP(); break;
          case SETT_MATILDA_CONF_IP : var = defSETT_MATILDA_CONF_IP(); break;
          case SETT_SVAHA_DATA_START_PORT : var = defSETT_SVAHA_DATA_START_PORT(); break;
          case SETT_SVAHA_DATA_PORT_COUNT: var = defSETT_SVAHA_DATA_PORT_COUNT(); break;


          case SETT_ZOMBIE_MSEC : var = defSETT_ZOMBIE_MSEC(); break;
          case SETT_TIME_2_LIVE : var = defSETT_TIME_2_LIVE(); break;

          case SETT_ADMIN_LOGIN   : var = defSETT_ADMIN_LOGIN(); break;
          case SETT_ADMIN_PASSWRD  : var = defSETT_ADMIN_PASSWRD(); break;

          case SETT_CERBERUS_PORT  : var = defSETT_CERBERUS_PORT(); break;

          default: continue; break;
          }

          settings.setValue(  valName4key( listKeys.at(i))   , var);



          qDebug() << "checkDefSett " << listKeys.at(i) << valName4key( listKeys.at(i))  << settings.contains( valName4key(listKeys.at(i)))
                   << settings.value(valName4key(listKeys.at(i))) << var;


    }
    settings.endGroup();    

}
//----------------------------------------------------------------------------------------------------------------------------
QString SettLoader4svaha::valName4key(const int &key)
{
    switch(key){
    case SETT_SVAHA_SERVICE_PORT : return "svaha-service-port";
    case SETT_MATILDA_DEV_IP : return "matilda-dev-ip";
    case SETT_MATILDA_CONF_IP : return "matilda-conf-ip";
    case SETT_SVAHA_DATA_START_PORT : return "start-port";
    case SETT_SVAHA_DATA_PORT_COUNT: return "port-count";


    case SETT_ZOMBIE_MSEC : return "zombie-msec";
    case SETT_TIME_2_LIVE : return "time2live";

    case SETT_ADMIN_LOGIN   : return "root_l";
    case SETT_ADMIN_PASSWRD  : return "root_p";

    case SETT_CERBERUS_PORT  : return "cerberus-port";

    }
    return "";
}
//----------------------------------------------------------------------------------------------------------------------------
QVariant SettLoader4svaha::loadOneSett(const int key)
{

    QSettings settings(path2sett(), QSettings::NativeFormat);
    if(settings.fileName().isEmpty()){
        qDebug() << "sett is empty " << settings.fileName();
        return false;
    }
     settings.beginGroup("svaha-conf");

     QString valKey = valName4key(key);

    switch(key){
    case SETT_SVAHA_SERVICE_PORT : return settings.value(valKey, defSETT_SVAHA_SERVICE_PORT());
    case SETT_MATILDA_DEV_IP : return settings.value(valKey, defSETT_MATILDA_DEV_IP());
    case SETT_MATILDA_CONF_IP : return settings.value(valKey, defSETT_MATILDA_CONF_IP());
    case SETT_SVAHA_DATA_START_PORT : return settings.value(valKey, defSETT_SVAHA_DATA_START_PORT());
    case SETT_SVAHA_DATA_PORT_COUNT: return settings.value(valKey, defSETT_SVAHA_DATA_PORT_COUNT());


    case SETT_ZOMBIE_MSEC : return settings.value(valKey, defSETT_ZOMBIE_MSEC());
    case SETT_TIME_2_LIVE : return settings.value(valKey, defSETT_TIME_2_LIVE());

    case SETT_ADMIN_LOGIN   : return settings.value(valKey, defSETT_ADMIN_LOGIN());
    case SETT_ADMIN_PASSWRD  : return settings.value(valKey, defSETT_ADMIN_PASSWRD());

    case SETT_CERBERUS_PORT  : return settings.value(valKey, defSETT_CERBERUS_PORT());

    }

    settings.endGroup();

    return QVariant();
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
//----------------------------------------------------------------------------------------------------------------------------
QString SettLoader4svaha::path2sett()
{

    return QString("%1/svaha.conf").arg(qApp->applicationDirPath());
}
//----------------------------------------------------------------------------------------------------------------------------
quint16 SettLoader4svaha::defSETT_SVAHA_SERVICE_PORT()
{
    return (quint16)65000;
}
//----------------------------------------------------------------------------------------------------------------------------
QString SettLoader4svaha::defSETT_MATILDA_DEV_IP()
{
    return "svaha.ddns.net";
}
//----------------------------------------------------------------------------------------------------------------------------
QString SettLoader4svaha::defSETT_MATILDA_CONF_IP()
{
    return "svaha.ddns.net";
}
//----------------------------------------------------------------------------------------------------------------------------
quint16 SettLoader4svaha::defSETT_SVAHA_DATA_START_PORT()
{
    return (quint16)50000;
}
//----------------------------------------------------------------------------------------------------------------------------
quint16 SettLoader4svaha::defSETT_SVAHA_DATA_PORT_COUNT()
{
    return (quint16)1000;
}
//----------------------------------------------------------------------------------------------------------------------------
int SettLoader4svaha::defSETT_ZOMBIE_MSEC()
{
    return (15 * 60 * 1000) ;
}
//----------------------------------------------------------------------------------------------------------------------------
int SettLoader4svaha::defSETT_TIME_2_LIVE()
{
    return (24 * 60 * 60 * 1000) ;
}
//----------------------------------------------------------------------------------------------------------------------------
QString SettLoader4svaha::defSETT_ADMIN_LOGIN()
{
    return "root";// QCryptographicHash::hash(QByteArray("root"), QCryptographicHash::Sha3_256);
}
//----------------------------------------------------------------------------------------------------------------------------
QString SettLoader4svaha::defSETT_ADMIN_PASSWRD()
{
    return "ChystaKrynytsa-Trutni-W.H.I.T.E";//QCryptographicHash::hash(QByteArray(""), QCryptographicHash::Sha3_256);
}
//----------------------------------------------------------------------------------------------------------------------------
quint16 SettLoader4svaha::defSETT_CERBERUS_PORT()
{
    return (quint16)50000;//після налаштування роутера замінити на 65001
}
//----------------------------------------------------------------------------------------------------------------------------
