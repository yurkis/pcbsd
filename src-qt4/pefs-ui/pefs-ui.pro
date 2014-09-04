TEMPLATE	= app
LANGUAGE	= C++

CONFIG	+= qt warn_on release

LIBS    += -L../libpcbsd -lQtSolutions_SingleApplication-head -lpcbsd-utils

INCLUDEPATH += ../libpcbsd/utils ../libpcbsd /usr/local/include

SOURCES += \
    main.cpp \
    TrayUI.cpp \
    backend.cpp

HEADERS += \
    TrayUI.h \
    backend.h
