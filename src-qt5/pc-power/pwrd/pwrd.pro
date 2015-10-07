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
    settingsreader.cpp \
    backlight.cpp \
    profilereader.cpp \
    ../common/src/serialize.cpp \
    ../common/src/protocol.cpp \
    intel_backlight.cpp

HEADERS += \
    pwrserver.h \
    sysctlutils.h \
    battery.h \
    settingsreader.h \
    backlight.h \
    profilereader.h \
    ../common/include/pwrdtypes.h \
    ../common/include/protocol.h \
    ../common/include/serialize.h \
    intel_backlight.h

INCLUDEPATH += ../common/include /usr/local/include/pwrd

QMAKE_LIBDIR = /usr/local/lib/qt5 /usr/local/lib
LIBS += -L/usr/local/lib

