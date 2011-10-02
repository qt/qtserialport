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
    abstractserialport_p.h \
    ringbuffer_p.h \
    serialport_p.h \
    serialportnotifier_p.h \
    serialportinfo_p.h

SOURCES += \
    serialport.cpp \
    serialport_p.cpp \
    serialportinfo.cpp

win32 {
    SOURCES += \
        serialport_p_win.cpp \
        serialportnotifier_p_win.cpp \
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

    SOURCES += \
        serialport_p_symbian.cpp \
        serialportnotifier_p_symbian.cpp \
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
        ttylocker_p_unix.h
    SOURCES += \
        serialport_p_unix.cpp \
        serialportnotifier_p_unix.cpp \
        ttylocker_p_unix.cpp

    macx {
        SOURCES += serialportinfo_mac.cpp
        LIBS += -framework IOKit -framework CoreFoundation
    } else {
        SOURCES += serialportinfo_unix.cpp
        contains( DEFINES, HAVE_UDEV ) {
            LIBS += -ludev
        }
    }
}




