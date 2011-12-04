TEMPLATE = lib
CONFIG += dll
#CONFIG += staticlib
QT -= gui
TARGET = SerialPort

win32 {
    DEFINES += SERIALPORT_BUILD SERIALPORT_SHARED
}

INCLUDEPATH += ../include
HEADERS += \
    ../include/serialport.h \
    ../include/serialportinfo.h

HEADERS += \
    serialport_p.h \
    ringbuffer_p.h \
    serialportengine_p.h \
    serialportinfo_p.h

SOURCES += \
    serialport.cpp \
    serialportinfo.cpp

win32 {
    HEADERS += \
        serialportengine_p_win.h
    SOURCES += \
        serialportengine_p_win.cpp \
        serialportinfo_win.cpp

    !wince*: LIBS += -lsetupapi -luuid -ladvapi32
}


symbian {
    MMP_RULES += EXPORTUNFROZEN
    #MMP_RULES += DEBUGGABLE_UDEBONLY
    TARGET.UID3 = 0xE7E62DFD
    TARGET.CAPABILITY =
    TARGET.EPOCALLOWDLLDATA = 1
    addFiles.sources = SerialPort.dll
    addFiles.path = !:/sys/bin
    DEPLOYMENT += addFiles
    
    # FIXME !!!
    #INCLUDEPATH += c:/Nokia/devices/Nokia_Symbian3_SDK_v1.0/epoc32/include/platform
    INCLUDEPATH += c:/QtSDK/Symbian/SDKs/Symbian3Qt473/epoc32/include/platform

    HEADERS += \
        serialportengine_p_symbian.h
    SOURCES += \
        serialportengine_p_symbian.cpp \
        serialportinfo_symbian.cpp
    LIBS += -leuser -lefsrv -lc32
}

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
    
    HEADERS += \
        ttylocker_p_unix.h \
        serialportengine_p_unix.h
    SOURCES += \
        ttylocker_p_unix.cpp \
        serialportengine_p_unix.cpp

    macx {
        SOURCES += serialportinfo_mac.cpp
        LIBS += -framework IOKit -framework CoreFoundation
    } else {
        SOURCES += serialportinfo_unix.cpp
        linux*:contains( DEFINES, HAVE_LIBUDEV ) {
            LIBS += -ludev
        }
    }
}




