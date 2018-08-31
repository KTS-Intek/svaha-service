QT += core network
QT -= gui

CONFIG += c++11

CONFIG += console
CONFIG -= app_bundle

# remove possible other optimization flags
QMAKE_CXXFLAGS_RELEASE -= -O1
QMAKE_CXXFLAGS_RELEASE -= -O2
#QMAKE_CXXFLAGS_RELEASE -= -O3
#QMAKE_CXXFLAGS += -Os

# add the desired -O3 if not present
QMAKE_CXXFLAGS_RELEASE *= -O3

linux-rasp-pi-g++:{
    DEFINES += ISRASPI=1
    target.path = /home/lazar/bin
}

linux-beagleboard-g++:{
#    target.path = /home/root
    target.path = /opt/matilda/bin
}

linux:{
#    target.path = /home/root
    target.path = /opt/matilda/bin
}
TARGET = svaha-service-bbb

INSTALLS += target

TEMPLATE = app

SOURCES += main.cpp \
    svahatrymachzjednannya.cpp \
    svahadladvoh.cpp \
    socketdlyatrymacha.cpp \
    socketprosto.cpp \
    cerber4matilda.cpp \
    cerber4matildasocket.cpp \
    settloader4svaha.cpp \
    backupmanager.cpp \
    checklocalfilesha1.cpp \
    service4uploadbackup.cpp \
    socket4uploadbackup.cpp \
    oldbackupcleaner.cpp \
    svahasharedmemorymanager.cpp \
    localsockettmplt.cpp \
    svahalocalsocket.cpp \
    matilda-bbb-src/shared/readjsonhelper.cpp \
    src/matilda/settloader4matilda.cpp \
    src/matilda/settloader4matildadefaults.cpp \
    src/shared/networkconverthelper.cpp \
    src/shared/sharedmemohelper.cpp \
    src/shared/sharedmemoprotocolhelper.cpp \
    matilda-bbb-src/shared/pathsresolver.cpp \
    src/matilda/serialporthelper.cpp \
    matilda-bbb-src/shared/macaddresshelper.cpp \
    matilda-bbb-src/shared/runprocess.cpp \
    src/matilda/matildaprotocolhelper.cpp \
    src/meter/numberconvertation.cpp \
    matilda-bbb-src/shared/ifacemanagerhelper.cpp \
    src/meter/meterpluginloader.cpp \
    src/meter/meteroperations.cpp

HEADERS += \
    svahatrymachzjednannya.h \
    svahadladvoh.h \
    socketdlyatrymacha.h \
    socketprosto.h \
    cerber4matilda.h \
    cerber4matildasocket.h \
    settloader4svaha.h \
    defcerberus.h \
    backupmanager.h \
    checklocalfilesha1.h \
    service4uploadbackup.h \
    socket4uploadbackup.h \
    svahadefine.h \
    oldbackupcleaner.h \
    svahasharedmemorymanager.h \
    localsockettmplt.h \
    svahalocalsocket.h \
    matilda-bbb-src/shared/readjsonhelper.h \
    src/matilda/moji_defy.h \
    src/matilda/matildalimits.h \
    src/matilda/settloader4matilda.h \
    src/matilda/settloader4matildadefaults.h \
    src/matilda/settloader4matildakeys.h \
    src/shared/networkconverthelper.h \
    src/shared/sharedmemohelper.h \
    src/shared/sharedmemoprotocolhelper.h \
    matilda-bbb-src/shared/pathsresolver.h \
    src/matilda/serialporthelper.h \
    matilda-bbb-src/shared/macaddresshelper.h \
    matilda-bbb-src/shared/runprocess.h \
    src/matilda/matildaprotocolhelper.h \
    src/meter/numberconvertation.h \
    matilda-bbb-src/shared/ifacemanagerhelper.h \
    src/meter/meterpluginloader.h \
    src/meter/meteroperations.h

DISTFILES += \
    LICENSE
