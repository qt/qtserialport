TEMPLATE = app
CONFIG += console
QT -= gui
OBJECTS_DIR = obj
MOC_DIR = moc

linux*:DEFINES += HAVE_LIBUDEV

INCLUDEPATH += \
    ../../include

HEADERS += \
    ../../include/serialport.h \
    ../../include/serialportinfo.h

include(../../src/src.pri)

SOURCES += main.cpp

CONFIG(debug, debug|release) {
    DESTDIR = debug
    TARGET = consoleinfod
} else {
    DESTDIR = release
    TARGET = consoleinfo
}

