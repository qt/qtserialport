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

symbian {
    MMP_RULES += EXPORTUNFROZEN
    #MMP_RULES += DEBUGGABLE_UDEBONLY
    TARGET.UID3 = 0xE7E62DFD
    TARGET.CAPABILITY =
    TARGET.EPOCALLOWDLLDATA = 1
    addFiles.sources = bug.dll
    addFiles.path = !:/sys/bin
    DEPLOYMENT += addFiles
    
    #INCLUDEPATH += c:/Nokia/devices/Nokia_Symbian3_SDK_v1.0/epoc32/include/platform
    INCLUDEPATH += c:/QtSDK/Symbian/SDKs/Symbian3Qt473/epoc32/include/platform

    SOURCES += \
        $$PWD/serialport_p_symbian.cpp \
        $$PWD/serialportnotifier_p_symbian.cpp \
        $$PWD/serialportinfo_symbian.cpp
    LIBS += -leuser -lefsrv -lc32

}

unix:!symbian {
    #maemo5 {
    #    target.path = /opt/usr/lib
    #} else {
    #    target.path = /usr/lib
    #}
    #INSTALLS += target
    
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
        contains( DEFINES, HAVE_UDEV ) {
            LIBS += -ludev
        }
    }
}




