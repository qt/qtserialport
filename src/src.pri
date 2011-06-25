INCLUDEPATH += ../include
HEADERS += \
    ../include/serialport.h \
    ../include/serialportinfo.h

HEADERS += \
    $$PWD/abstractserialport_p.h \
    $$PWD/serialport_p.h \
    $$PWD/serialportinfo_p.h


SOURCES += \
    $$PWD/serialport.cpp \
    $$PWD/serialportinfo.cpp
    
win32 {
    SOURCES += \
        $$PWD/serialport_p_win.cpp \
        $$PWD/serialportinfo_win.cpp

    LIBS += -lsetupapi -luuid -ladvapi32
}

unix { 
    SOURCES += \
        $$PWD/serialport_p_unix.cpp

    macx {
        SOURCES += $$PWD/serialportinfo_mac.cpp
        LIBS += -framework IOKit -framework CoreFoundation
    } else {
        SOURCES += $$PWD/serialportinfo_unix.cpp
        LIBS += -ludev
    }
}
