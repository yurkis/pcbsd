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
