INCLUDEPATH += ../include
HEADERS += \
    ../include/serialport.h \
    ../include/serialportinfo.h

HEADERS += \
    abstractserialport_p.h \
    serialport_p.h \
    serialportinfo_p.h


SOURCES += \
    serialport.cpp \
    serialportinfo.cpp
    
win32 {
    SOURCES += \
        serialport_p_win.cpp \
        serialportinfo_win.cpp

    LIBS += -lsetupapi -luuid -ladvapi32
}

unix { 
    SOURCES += \
        serialport_p_unix.cpp

    macx {
        SOURCES += serialportinfo_mac.cpp
        LIBS += -framework IOKit -framework CoreFoundation
    } else {
        SOURCES += serialportinfo_unix.cpp
        #LIBS += -ludev
    }
}
