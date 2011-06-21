HEADERS += include/serialport.h \
    src/abstractserialport_p.h \
    src/serialport_p.h \
    src/qserialdevice_global.h \

    
SOURCES += src/serialport.cpp
    
win32 {
    SOURCES += src/serialport_p_win.cpp
}
unix { 
    SOURCES += src/serialport_p_unix.cpp
}
