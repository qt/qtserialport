TEMPLATE = app
CONFIG += console
QT -= gui
OBJECTS_DIR = obj
MOC_DIR = moc

unix {
    !macx:DEFINES += HAVE_UDEV
}

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

