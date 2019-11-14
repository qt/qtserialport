# Generated from cenumerator.pro.

cmake_minimum_required(VERSION 3.14)
project(cenumerator LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/serialport/cenumerator")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS SerialPort)

add_executable(cenumerator
    main.cpp
)
target_link_libraries(cenumerator PUBLIC
    Qt::Core
    Qt::SerialPort
)

install(TARGETS cenumerator
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
