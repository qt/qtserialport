# Generated from enumerator.pro.

cmake_minimum_required(VERSION 3.14)
project(enumerator LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/serialport/enumerator")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)
find_package(Qt6 COMPONENTS SerialPort)

add_qt_gui_executable(enumerator
    main.cpp
)
target_link_libraries(enumerator PUBLIC
    Qt::Core
    Qt::Gui
    Qt::SerialPort
    Qt::Widgets
)

install(TARGETS enumerator
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
