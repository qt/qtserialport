cmake_minimum_required(VERSION 3.16)
project(cenumerator LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/serialport/cenumerator")

find_package(Qt6 REQUIRED COMPONENTS Core SerialPort)

qt_add_executable(cenumerator
    main.cpp
)

set_target_properties(cenumerator PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE FALSE
)

target_link_libraries(cenumerator PRIVATE
    Qt::Core
    Qt::SerialPort
)

install(TARGETS cenumerator
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
