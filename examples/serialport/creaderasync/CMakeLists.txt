# Generated from creaderasync.pro.

cmake_minimum_required(VERSION 3.14)
project(creaderasync LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS SerialPort)

add_executable(creaderasync
    main.cpp
    serialportreader.cpp serialportreader.h
)
target_link_libraries(creaderasync PUBLIC
    Qt::Core
    Qt::SerialPort
)

install(TARGETS creaderasync
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
