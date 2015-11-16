QT += core network
QT -= gui

TARGET = pwrd
CONFIG += console c++11
#CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    pwrserver.cpp \
    sysctlutils.cpp \
    hw/battery.cpp \
    settingsreader.cpp \
    hw/backlight.cpp \
    profilereader.cpp \
    ../common/src/serialize.cpp \
    ../common/src/protocol.cpp \
    hw/intel_backlight.cpp \
    hw/buttons.cpp \
    comm_handlers.cpp \
    hw/sleep.cpp \
    hw/fakebatt.cpp

HEADERS += \
    pwrserver.h \
    sysctlutils.h \
    hw/battery.h \
    settingsreader.h \
    hw/backlight.h \
    profilereader.h \
    ../common/include/pwrdtypes.h \
    ../common/include/protocol.h \
    ../common/include/serialize.h \
    hw/intel_backlight.h \
    hw/buttons.h \
    hw/sleep.h

INCLUDEPATH += ../common/include /usr/local/include/pwrd

QMAKE_LIBDIR = /usr/local/lib/qt5 /usr/local/lib
LIBS += -L/usr/local/lib

target.path=/usr/local/sbin

cleanprofiles.path=/usr/local/share/pcbsd/pwrd/
cleanprofiles.extra=rm -rf $(INSTALL_ROOT)/usr/local/share/pcbsd/pwrd/default

mkdirconf.path=/usr/local/share/pcbsd/pwrd/
mkdirconf.extra=mkdir -p $(INSTALL_ROOT)/usr/local/share/pcbsd/pwrd/

cpconfig.path=/usr/local/share/pcbsd/pwrd/
cpconfig.extra=tar cvf - -C conf . 2>/dev/null | tar xvf - -C $(INSTALL_ROOT)/usr/local/share/pcbsd/pwrd/ 2>/dev/null
#&& chmod -R 0555 $(INSTALL_ROOT)/usr/local/share/pcbsd/pc-power/ 2>/dev/null

rcd.path=/usr/local/etc/rc.d
rcd.extra=cp rc.d/pwrd $(INSTALL_ROOT)/usr/local/etc/rc.d/ && chmod -R 0555 $(INSTALL_ROOT)/usr/local/etc/rc.d/ pwrd


INSTALLS += target cleanprofiles mkdirconf cpconfig rcd
