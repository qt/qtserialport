TEMPLATE = lib
CONFIG += dll
QT -= gui
TARGET = SerialPort

include(src.pri)

win32 {
    DEFINES += SERIALPORT_BUILD SERIALPORT_SHARED
    LIBS += -lsetupapi -luuid -ladvapi32
}

#unix:!macx {
#    LIBS += -ludev
#}

macx {
    LIBS += -framework IOKit -framework CoreFoundation
}
