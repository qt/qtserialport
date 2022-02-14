cmake_minimum_required(VERSION 3.16)
project(creaderasync LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/serialport/creaderasync")

find_package(Qt6 REQUIRED COMPONENTS Core SerialPort)

qt_add_executable(creaderasync
    main.cpp
    serialportreader.cpp serialportreader.h
)

set_target_properties(creaderasync PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE FALSE
)

target_link_libraries(creaderasync PRIVATE
    Qt::Core
    Qt::SerialPort
)

install(TARGETS creaderasync
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
