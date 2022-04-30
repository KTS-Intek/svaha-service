QT += core network
QT -= gui

CONFIG += c++11

CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app


VERSION = 0.0.3

#DEFINES += APPLCTN_NAME=\\\"quick-collect\\\" it is only for GUI
DEFINES += "MYAPPNAME=\"\\\"svaha-service\\\"\""
DEFINES += "MYAPPOWNER=\"\\\"KTS-Intek Ltd\\\"\""
DEFINES += "MYAPPOWNERSITE=\"\\\"https://kts-intek.com\\\"\""

DEFINES += ENABLE_SETTLOADER4MATILDA
DEFINES += DONOTINCLUDEZBYRATORSHARED


DEFINES += QT_DEPRECATED_WARNINGS

## remove possible other optimization flags
#QMAKE_CXXFLAGS_RELEASE -= -O1
#QMAKE_CXXFLAGS_RELEASE -= -O2
##QMAKE_CXXFLAGS_RELEASE -= -O3
##QMAKE_CXXFLAGS += -Os

## add the desired -O3 if not present
#QMAKE_CXXFLAGS_RELEASE *= -O3

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

include(../../Matilda-units/matilda-base/type-converter/type-converter.pri)

include(../../Matilda-units/ipc/localsockets/localsockets.pri)
include(../../Matilda-units/ipc/sharedmemory/sharedmemory.pri)
include(../../Matilda-units/matilda-bbb/matilda-bbb-settings/matilda-bbb-settings.pri)
include(../../Matilda-units/matilda-base/MatildaIO/MatildaIO.pri)
include(../../Matilda-units/matilda-base/matilda-bbb-shared/matilda-bbb-shared.pri)

include(../../Matilda-units/matilda-bbb/matilda-bbb-m2m-server/matilda-bbb-m2m-server.pri)


SOURCES += main.cpp \
    m2m-service-src/backup/m2mbackupmanager.cpp \
    m2m-service-src/backup/m2mlocalfilesha1checker.cpp \
    m2m-service-src/backup/m2moldbackupskiller.cpp \
    m2m-service-src/connection-holder/m2mconnholderdecoder.cpp \
    m2m-service-src/connection-holder/m2mconnholderserver.cpp \
    m2m-service-src/connection-holder/m2mconnholderserverbase.cpp \
    m2m-service-src/connection-holder/m2mconnholdersocket.cpp \
    m2m-service-src/data-transmission/m2mbackupconndecoder.cpp \
    m2m-service-src/data-transmission/m2mbackupserver.cpp \
    m2m-service-src/data-transmission/m2mbackupsocket.cpp \
    m2m-service-src/data-transmission/m2mdatasocket.cpp \
    m2m-service-src/data-transmission/m2mserverfortwo.cpp \
    m2m-service-src/main/m2mapplogs.cpp \
    m2m-service-src/main/m2mglobalmethods.cpp \
    m2m-service-src/main/m2mlocalsocket.cpp \
    m2m-service-src/main/m2mresourcemanager.cpp \
    m2m-service-src/main/m2msharedmemorywriter.cpp



HEADERS += \
    m2m-service-src/backup/m2mbackupmanager.h \
    m2m-service-src/backup/m2mlocalfilesha1checker.h \
    m2m-service-src/backup/m2moldbackupskiller.h \
    m2m-service-src/connection-holder/m2mconnholderdecoder.h \
    m2m-service-src/connection-holder/m2mconnholderserver.h \
    m2m-service-src/connection-holder/m2mconnholderserverbase.h \
    m2m-service-src/connection-holder/m2mconnholdersocket.h \
    m2m-service-src/data-transmission/m2mbackupconndecoder.h \
    m2m-service-src/data-transmission/m2mbackupserver.h \
    m2m-service-src/data-transmission/m2mbackupsocket.h \
    m2m-service-src/data-transmission/m2mdatasocket.h \
    m2m-service-src/data-transmission/m2mserverfortwo.h \
    m2m-service-src/main/m2mapplogs.h \
    m2m-service-src/main/m2mglobalmethods.h \
    m2m-service-src/main/m2mlocalsocket.h \
    m2m-service-src/main/m2mresourcemanager.h \
    m2m-service-src/main/m2mservertypes.h \
    m2m-service-src/main/m2msharedmemorywriter.h


DISTFILES += \
    LICENSE
