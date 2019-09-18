# Generated from serialport.pro.

qt_find_package(Libudev PROVIDED_TARGETS PkgConfig::Libudev) # special case

#####################################################################
## SerialPort Module:
#####################################################################

add_qt_module(SerialPort
    SOURCES
        qserialport.cpp qserialport.h qserialport_p.h
        qserialportglobal.h
        qserialportinfo.cpp qserialportinfo.h qserialportinfo_p.h
    LIBRARIES
        Qt::CorePrivate
    PUBLIC_LIBRARIES
        Qt::Core
)

#### Keys ignored in scope 1:.:.:serialport.pro:<TRUE>:
# _LOADED = "qt_module"

## Scopes:
#####################################################################

# special case begin
# Temporarily remove until ntddmodm config test is ported
#extend_target(SerialPort CONDITION QT_FEATURE_ntddmodm
#    DEFINES
#        QT_NO_REDEFINE_GUID_DEVINTERFACE_MODEM
#)
# special case end

extend_target(SerialPort CONDITION QT_FEATURE_libudev AND UNIX
    DEFINES
        LINK_LIBUDEV
    LIBRARIES # special case
        PkgConfig::Libudev # special case
)

extend_target(SerialPort CONDITION WIN32
    SOURCES
        qserialport_win.cpp
        qserialportinfo_win.cpp
        qtntdll_p.h # special case
    LIBRARIES
        advapi32
        setupapi
)

extend_target(SerialPort CONDITION UNIX
    SOURCES
        qserialport_unix.cpp
)

extend_target(SerialPort CONDITION APPLE_OSX
    SOURCES
        qserialportinfo_osx.cpp
    LIBRARIES
        ${FWCoreFoundation}
        ${FWIOKit}
)

extend_target(SerialPort CONDITION FREEBSD
    SOURCES
        qserialportinfo_freebsd.cpp
)

extend_target(SerialPort CONDITION UNIX AND NOT APPLE_OSX AND NOT FREEBSD
    SOURCES
        qserialportinfo_unix.cpp
)
add_qt_docs(
    doc/qtserialport.qdocconf
)

