# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(terminal LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/serialport/terminal")

find_package(Qt6 REQUIRED COMPONENTS Core Gui SerialPort Widgets)

qt_standard_project_setup()

qt_add_executable(terminal
    console.cpp console.h
    main.cpp
    mainwindow.cpp mainwindow.h mainwindow.ui
    settingsdialog.cpp settingsdialog.h settingsdialog.ui
)

set_target_properties(terminal PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(terminal PRIVATE
    Qt::Core
    Qt::Gui
    Qt::SerialPort
    Qt::Widgets
)

# Resources:
set(terminal_resource_files
    "images/application-exit.png"
    "images/clear.png"
    "images/connect.png"
    "images/disconnect.png"
    "images/settings.png"
)

qt_add_resources(terminal "terminal"
    PREFIX
        "/"
    FILES
        ${terminal_resource_files}
)

install(TARGETS terminal
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
