cmake_minimum_required(VERSION 3.16)
project(cwritersync LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/serialport/cwritersync")

find_package(Qt6 REQUIRED COMPONENTS Core SerialPort)

qt_add_executable(cwritersync
    main.cpp
)

set_target_properties(cwritersync PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE FALSE
)

target_link_libraries(cwritersync PRIVATE
    Qt::Core
    Qt::SerialPort
)

install(TARGETS cwritersync
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
