HEADERS += \
    serialport_p.h \
    serialportinfo_p.h
    
SOURCES += \
    src/serialport.cpp \
    src/serialportinfo.cpp
    
win32 {
    HEADERS += \
        serialport_p_win.h
    SOURCES += \
        src/serialport_p_win.cpp \
        src/serialportinfo_p_win.cpp
}
unix { 
    HEADERS += \
        serialport_p_unix.h
    SOURCES += \
        src/serialport_p_unix.cpp

    macx {
        SOURCES += src/serialportinfo_p_mac.cpp
    }
    !macx {
        SOURCES += src/serialportinfo_p_unix.cpp
    }
}
