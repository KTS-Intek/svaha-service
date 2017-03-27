QT += core network
QT -= gui

CONFIG += c++11

CONFIG += console
CONFIG -= app_bundle


linux-rasp-pi-g++:{
    target.path = /home/lazar/bin
    INSTALLS += target
}

linux-beagleboard-g++:{
    target.path = /home/root
    INSTALLS += target
}
TARGET = svaha-service

INSTALLS += target

TEMPLATE = app

SOURCES += main.cpp \
    svahatrymachzjednannya.cpp \
    svahadladvoh.cpp \
    socketdlyatrymacha.cpp \
    socketprosto.cpp \
    cerber4matilda.cpp \
    cerber4matildasocket.cpp \
    settloader4svaha.cpp

HEADERS += \
    svahatrymachzjednannya.h \
    svahadladvoh.h \
    socketdlyatrymacha.h \
    socketprosto.h \
    cerber4matilda.h \
    ../matilda-bbb/moji_defy.h \
    cerber4matildasocket.h \
    settloader4svaha.h \
    defcerberus.h

DISTFILES += \
    LICENSE
