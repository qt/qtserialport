# Generated from qtserialport.pro.

cmake_minimum_required(VERSION 3.15.0)

project(QtSerialPort
    VERSION 6.0.0
    DESCRIPTION "Qt SerialPort Libraries"
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX C
)

find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS BuildInternals Core) # special case
find_package(Qt6 ${PROJECT_VERSION} CONFIG OPTIONAL_COMPONENTS Xml) # special case

if(INTEGRITY)
    message(NOTICE "Skipping the build as the condition \"NOT INTEGRITY\" is not met.")
    return()
endif()
if(VXWORKS)
    message(NOTICE "Skipping the build as the condition \"NOT VXWORKS\" is not met.")
    return()
endif()
if(WINRT)
    message(NOTICE "Skipping the build as the condition \"NOT WINRT\" is not met.")
    return()
endif()
if(APPLE_UIKIT)
    message(NOTICE "Skipping the build as the condition \"NOT APPLE_UIKIT\" is not met.")
    return()
endif()
if(EMSCRIPTEN)
    message(NOTICE "Skipping the build as the condition \"NOT EMSCRIPTEN\" is not met.")
    return()
endif()
qt_build_repo()
