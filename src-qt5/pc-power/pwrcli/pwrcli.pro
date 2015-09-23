QT += core network
QT -= gui

TARGET = pwrcli
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    pwrcli.cpp \
    QPWRDClient.cpp \
    ../common/src/protocol.cpp \
    ../common/src/serialize.cpp

HEADERS += \
    pwrcli.h \
    QPWRDClient.h \
    ../common/include/protocol.h \
    ../common/include/pwrdtypes.h \
    ../common/include/serialize.h

INCLUDEPATH += ../common/include /usr/local/include/pwrd
LIBS += -L/usr/local/lib

