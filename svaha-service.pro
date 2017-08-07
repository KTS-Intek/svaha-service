QT += core network
QT -= gui

CONFIG += c++11

CONFIG += console
CONFIG -= app_bundle


linux-rasp-pi-g++:{
    DEFINES += ISRASPI=1
    target.path = /home/lazar/bin
}

linux-beagleboard-g++:{
    target.path = /home/root
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
    readjsonhelper.cpp \
    backupmanager.cpp \
    matildasynchelper.cpp \
    matildaprotocolhelper.cpp \
    checklocalfilesha1.cpp \
    service4uploadbackup.cpp \
    socket4uploadbackup.cpp \
    settloader4matilda.cpp

HEADERS += \
    svahatrymachzjednannya.h \
    svahadladvoh.h \
    socketdlyatrymacha.h \
    socketprosto.h \
    cerber4matilda.h \
    ../matilda-bbb/moji_defy.h \
    cerber4matildasocket.h \
    settloader4svaha.h \
    defcerberus.h \
    readjsonhelper.h \
    backupmanager.h \
    matildasynchelper.h \
    matildaprotocolhelper.h \
    checklocalfilesha1.h \
    service4uploadbackup.h \
    socket4uploadbackup.h \
    settloader4matilda.h \
    svahadefine.h

DISTFILES += \
    LICENSE
