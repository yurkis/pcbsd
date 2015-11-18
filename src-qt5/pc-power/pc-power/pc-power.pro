#-------------------------------------------------
#
# Project created by QtCreator 2015-11-05T16:09:36
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = pc-power
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    ../libpwrd/QPWRDClient.cpp \
    widgets/widgetbatteryhw.cpp \
    widgets/widgetbattery.cpp \
    widgets/widgetbacklight.cpp \
    ../common/src/protocol.cpp \
    ../common/src/serialize.cpp \
    ../libpwrd/QPWRDEvents.cpp \
    widgets/widgetsleepbuttons.cpp

HEADERS  += mainwindow.h \
    ../libpwrd/QPWRDClient.h \
    ../common/include/pwrdtypes.h \
    widgets/widgetbatteryhw.h \
    widgets/widgetbattery.h \
    widgets/widgetbacklight.h \
    ../libpwrd/QPWRDEvents.h \
    ../common/include/protocol.h \
    widgets/widgetsleepbuttons.h

FORMS    += mainwindow.ui \
    widgets/widgetbatteryhw.ui \
    widgets/widgetbattery.ui \
    widgets/widgetbacklight.ui \
    widgets/widgetsleepbuttons.ui

INCLUDEPATH += ../common/include ../libpwrd  /usr/local/include/pwrd
LIBS += -L/usr/local/lib

RESOURCES += \
    pc-power.qrc

DISTFILES +=
