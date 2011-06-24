TEMPLATE = lib
CONFIG += dll
QT -= gui
TARGET = SerialPort

include(src.pri)

win32 {
    DEFINES += SERIALPORT_BUILD SERIALPORT_SHARED
}
