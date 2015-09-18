QT += core network
QT -= gui

TARGET = pwrd
CONFIG += console
#CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    pwrserver.cpp \
    sysctlutils.cpp \
    battery.cpp \
    settingsreader.cpp

HEADERS += \
    pwrserver.h \
    pwrd.h \
    sysctlutils.h \
    battery.h \
    settingsreader.h

QMAKE_LIBDIR = /usr/local/lib/qt5 /usr/local/lib
LIBS += -L/usr/local/lib

