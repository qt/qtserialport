# Generated from cwriterasync.pro.

cmake_minimum_required(VERSION 3.14)
project(cwriterasync LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/serialport/cwriterasync")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS SerialPort)

add_executable(cwriterasync
    main.cpp
    serialportwriter.cpp serialportwriter.h
)
target_link_libraries(cwriterasync PUBLIC
    Qt::Core
    Qt::SerialPort
)

install(TARGETS cwriterasync
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
