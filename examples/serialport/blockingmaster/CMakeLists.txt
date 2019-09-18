# Generated from blockingmaster.pro.

cmake_minimum_required(VERSION 3.14)
project(blockingmaster LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)
find_package(Qt6 COMPONENTS SerialPort)

add_qt_gui_executable(blockingmaster
    dialog.cpp dialog.h
    main.cpp
    masterthread.cpp masterthread.h
)
target_link_libraries(blockingmaster PUBLIC
    Qt::Core
    Qt::Gui
    Qt::SerialPort
    Qt::Widgets
)

install(TARGETS blockingmaster
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
