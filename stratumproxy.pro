QT += core network

QT -= gui

TARGET = stratumproxy

CONFIG += console

CONFIG += static

CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    alogger.cpp \
    aunixsignalhandler.cpp \
    aproxymachine.cpp \
    aconfighandler.cpp \
    apoolmonitor.cpp \
    apoolchecker.cpp \
    amainpoolhandler.cpp \
    abackuppoolhandler.cpp

HEADERS += \
    alogger.h \
    asingleton.h \
    aunixsignalhandler.h \
    aproxymachine.h \
    aconfighandler.h \
    apoolmonitor.h \
    apoolchecker.h \
    amainpoolhandler.h \
    abackuppoolhandler.h

OTHER_FILES += \
    README.md
