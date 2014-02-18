QT += core network

QT -= gui

TARGET = stratumproxy

CONFIG += console

CONFIG += static

CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    adaemon.cpp \
    alogger.cpp

HEADERS += \
    adaemon.h \
    alogger.h \
    asingleton.h
