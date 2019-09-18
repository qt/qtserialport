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

# special case begin
if (INTEGRITY OR WINRT OR APPLE_UIKIT OR VXWORKS OR WASM)
   return()
endif()
# special case end

qt_build_repo()
