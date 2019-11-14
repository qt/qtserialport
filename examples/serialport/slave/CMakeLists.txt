# Generated from slave.pro.

cmake_minimum_required(VERSION 3.14)
project(slave LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/serialport/slave")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)

add_qt_gui_executable(slave
    dialog.cpp dialog.h
    main.cpp
)
target_link_libraries(slave PUBLIC
    Qt::Core
    Qt::Gui
)

if((QT_MAJOR_VERSION GREATER 4))
    target_link_libraries(slave PUBLIC
        Qt::SerialPort
        Qt::Widgets
    )
endif()

install(TARGETS slave
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
