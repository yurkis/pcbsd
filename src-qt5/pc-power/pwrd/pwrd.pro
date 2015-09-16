QT += core network
QT -= gui

TARGET = pwrd
CONFIG += console
#CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    pwrserver.cpp \
    acpiinfo.cpp

HEADERS += \
    pwrserver.h \
    pwrd.h \
    acpiinfo.h

QMAKE_LIBDIR = /usr/local/lib/qt5 /usr/local/lib
LIBS += -L/usr/local/lib

