INCLUDEPATH += $$PWD

linux*:DEFINES += HAVE_LIBUDEV

greaterThan(QT_MAJOR_VERSION, 4) {
    HEADERS += qtaddonserialportversion.h
}

PUBLIC_HEADERS += \
    $$PWD/serialport-global.h \
    $$PWD/serialport.h \
    $$PWD/serialportinfo.h

PRIVATE_HEADERS += \
    $$PWD/serialport_p.h \
    $$PWD/serialportengine_p.h \
    $$PWD/serialportinfo_p.h

SOURCES += \
    $$PWD/serialport.cpp \
    $$PWD/serialportinfo.cpp


win32 {
    PRIVATE_HEADERS += \
        $$PWD/serialportengine_win_p.h
    SOURCES += \
        $$PWD/serialportengine_win.cpp \
        $$PWD/serialportinfo_win.cpp

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

    PRIVATE_HEADERS += \
        $$PWD/serialportengine_symbian_p.h
    SOURCES += \
        $$PWD/serialportengine_symbian.cpp \
        $$PWD/serialportinfo_symbian.cpp
    LIBS += -leuser -lefsrv -lc32
}

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target

    PRIVATE_HEADERS += \
        $$PWD/ttylocker_unix_p.h \
        $$PWD/serialportengine_unix_p.h
    SOURCES += \
        $$PWD/ttylocker_unix.cpp \
        $$PWD/serialportengine_unix.cpp

    macx {
        SOURCES += $$PWD/serialportinfo_mac.cpp
        LIBS += -framework IOKit -framework CoreFoundation
    } else {
        SOURCES += $$PWD/serialportinfo_unix.cpp
        linux*:contains( DEFINES, HAVE_LIBUDEV ) {
            LIBS += -ludev
        }
    }
}

HEADERS += $$PUBLIC_HEADERS $$PRIVATE_HEADERS
