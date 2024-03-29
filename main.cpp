/****************************************************************************
**
**   Copyright © 2016-2021 The KTS-INTEK Ltd.
**   Contact: https://www.kts-intek.com
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
#include <QCoreApplication>


#include "m2m-service-src/main/m2mresourcemanager.h"

//#include "svahatrymachzjednannya.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    M2MResourceManager manager;
    manager.startEverything(qApp->arguments().contains("-vv"));

//    SvahaTrymachZjednannya svaha;
//    svaha.initObjects();
    return a.exec();
}
