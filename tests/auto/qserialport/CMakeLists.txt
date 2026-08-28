# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qserialport LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## tst_qserialport Binary:
#####################################################################

qt_internal_add_test(tst_qserialport
    SOURCES
        tst_qserialport.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::SerialPort
        Qt::Test
        Qt::TestPrivate
)
