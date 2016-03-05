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
    widgets/widgetsleepbuttons.cpp \
    dialogs/connecterrordialog.cpp \
    dialogs/profileeditdialog.cpp \
    widgets/widgetbtnsettings.cpp \
    dialogs/notification.cpp

HEADERS  += mainwindow.h \
    ../libpwrd/QPWRDClient.h \
    ../common/include/pwrdtypes.h \
    widgets/widgetbatteryhw.h \
    widgets/widgetbattery.h \
    widgets/widgetbacklight.h \
    ../libpwrd/QPWRDEvents.h \
    ../common/include/protocol.h \
    widgets/widgetsleepbuttons.h \
    dialogs/connecterrordialog.h \
    ../common/include/serialize.h \
    dialogs/profileeditdialog.h \
    widgets/widgetbtnsettings.h \
    ssdescription.h \
    dialogs/notification.h

FORMS    += mainwindow.ui \
    widgets/widgetbatteryhw.ui \
    widgets/widgetbattery.ui \
    widgets/widgetbacklight.ui \
    widgets/widgetsleepbuttons.ui \
    dialogs/connecterrordialog.ui \
    dialogs/profileeditdialog.ui \
    widgets/widgetbtnsettings.ui \
    dialogs/notification.ui

INCLUDEPATH += ../common/include ../libpwrd  /usr/local/include/pwrd
LIBS += -L/usr/local/lib

RESOURCES += \
    pc-power.qrc

target.path=/usr/local/bin/

desktop.path=/usr/local/share/applications/
desktop.files=pc-power.desktop

appicon.path=/usr/local/share/pcbsd/icons/
appicon.files=pc-power.png

INSTALLS += target appicon desktop



DISTFILES +=
