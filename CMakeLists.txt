# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

include(.cmake.conf)
project(QtSerialPort
    VERSION "${QT_REPO_MODULE_VERSION}"
    DESCRIPTION "Qt SerialPort Libraries"
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX C
)

set(QT_NO_INTERNAL_COMPATIBILITY_FUNCTIONS TRUE)

find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS BuildInternals Core)
find_package(Qt6 ${PROJECT_VERSION} CONFIG OPTIONAL_COMPONENTS Gui Widgets)
qt_internal_project_setup()

if(INTEGRITY)
    message(NOTICE "Skipping the build as the condition \"NOT INTEGRITY\" is not met.")
    return()
endif()
if(VXWORKS)
    message(NOTICE "Skipping the build as the condition \"NOT VXWORKS\" is not met.")
    return()
endif()
if(WINRT)
    message(NOTICE "Skipping the build as the condition \"NOT WINRT\" is not met.")
    return()
endif()
if(UIKIT)
    message(NOTICE "Skipping the build as the condition \"NOT UIKIT\" is not met.")
    return()
endif()
if(EMSCRIPTEN)
    message(NOTICE "Skipping the build as the condition \"NOT EMSCRIPTEN\" is not met.")
    return()
endif()
qt_build_repo()
