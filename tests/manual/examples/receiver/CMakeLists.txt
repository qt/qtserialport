# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(receiver LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui SerialPort Widgets)

qt_standard_project_setup()

qt_add_executable(receiver
    dialog.cpp dialog.h
    main.cpp
)

set_target_properties(receiver PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(receiver PRIVATE
    Qt::Core
    Qt::Gui
    Qt::SerialPort
    Qt::Widgets
)
