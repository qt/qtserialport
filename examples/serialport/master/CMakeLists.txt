# Generated from master.pro.

cmake_minimum_required(VERSION 3.14)
project(master LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)

add_qt_gui_executable(master
    dialog.cpp dialog.h
    main.cpp
)
target_link_libraries(master PUBLIC
    Qt::Core
    Qt::Gui
)

install(TARGETS master
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
