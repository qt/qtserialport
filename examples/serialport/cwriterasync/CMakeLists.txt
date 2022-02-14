cmake_minimum_required(VERSION 3.16)
project(cwriterasync LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/serialport/cwriterasync")

find_package(Qt6 REQUIRED COMPONENTS Core SerialPort)

qt_add_executable(cwriterasync
    main.cpp
    serialportwriter.cpp serialportwriter.h
)

set_target_properties(cwriterasync PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE FALSE
)

target_link_libraries(cwriterasync PRIVATE
    Qt::Core
    Qt::SerialPort
)

install(TARGETS cwriterasync
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
