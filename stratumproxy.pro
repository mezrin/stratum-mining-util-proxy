QT += core network

QT -= gui

TARGET = stratumproxy

CONFIG += console

CONFIG += static

CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    adaemon.cpp

HEADERS += \
    adaemon.h
