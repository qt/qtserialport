# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## SerialPort Module:
#####################################################################
qt_find_package(Libudev)

qt_internal_add_module(SerialPort
    SOURCES
        qserialport.cpp qserialport.h qserialport_p.h
        qserialportglobal.h
        qserialportinfo.cpp qserialportinfo.h qserialportinfo_p.h
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}
    LIBRARIES
        Qt::CorePrivate
    PUBLIC_LIBRARIES
        Qt::Core
    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
    GENERATE_CPP_EXPORTS
)

## Scopes:
#####################################################################

qt_internal_extend_target(SerialPort CONDITION QT_FEATURE_libudev AND UNIX
   DEFINES
       LINK_LIBUDEV
   LIBRARIES
       PkgConfig::Libudev
)

qt_internal_extend_target(SerialPort CONDITION WIN32
    SOURCES
        qserialport_win.cpp
        qserialportinfo_win.cpp
        qwinoverlappedionotifier.cpp
        qwinoverlappedionotifier_p.h
    LIBRARIES
        advapi32
        setupapi
)

qt_internal_extend_target(SerialPort CONDITION UNIX
    SOURCES
        qserialport_unix.cpp
)

qt_internal_extend_target(SerialPort CONDITION MACOS
    SOURCES
        qserialportinfo_osx.cpp
    LIBRARIES
        ${FWCoreFoundation}
        ${FWIOKit}
)

qt_internal_extend_target(SerialPort CONDITION FREEBSD
    SOURCES
        qserialportinfo_freebsd.cpp
)

qt_internal_extend_target(SerialPort CONDITION UNIX AND NOT FREEBSD AND NOT MACOS
    SOURCES
        qserialportinfo_unix.cpp
)

qt_internal_add_docs(SerialPort
    doc/qtserialport.qdocconf
)

