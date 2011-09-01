INCLUDEPATH += ../include
HEADERS += \
    ../include/serialport.h \
    ../include/serialportinfo.h

HEADERS += \
    $$PWD/abstractserialport_p.h \
    $$PWD/ringbuffer_p.h \
    $$PWD/serialport_p.h \
    $$PWD/serialportnotifier_p.h \
    $$PWD/serialportinfo_p.h


SOURCES += \
    $$PWD/serialport.cpp \
    $$PWD/serialport_p.cpp \
    $$PWD/serialportinfo.cpp

win32 {
    SOURCES += \
        $$PWD/serialport_p_win.cpp \
        $$PWD/serialportnotifier_p_win.cpp \
        $$PWD/serialportinfo_win.cpp

    !wince*: LIBS += -lsetupapi -luuid -ladvapi32
}

unix {
    HEADERS += \
        $$PWD/ttylocker_p_unix.h
    SOURCES += \
        $$PWD/serialport_p_unix.cpp \
        $$PWD/serialportnotifier_p_unix.cpp \
        $$PWD/ttylocker_p_unix.cpp

    macx {
        SOURCES += $$PWD/serialportinfo_mac.cpp
        LIBS += -framework IOKit -framework CoreFoundation
    } else {
        SOURCES += $$PWD/serialportinfo_unix.cpp
        #LIBS += -ludev
    }
}
