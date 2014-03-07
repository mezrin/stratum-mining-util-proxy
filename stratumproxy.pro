QT += core network

QT -= gui

TARGET = stratumproxy

CONFIG += console

CONFIG += static

CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    adaemon.cpp \
    alogger.cpp \
    aunixsignalhandler.cpp \
    aproxymachine.cpp \
    aconfighandler.cpp \
    apoolmonitor.cpp

HEADERS += \
    adaemon.h \
    alogger.h \
    asingleton.h \
    aunixsignalhandler.h \
    aproxymachine.h \
    aconfighandler.h \
    apoolmonitor.h

OTHER_FILES += \
    README.md
