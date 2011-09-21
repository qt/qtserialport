TEMPLATE = app
CONFIG += console
QT -= gui
OBJECTS_DIR = obj
MOC_DIR = moc

unix {
    !macx:DEFINES += HAVE_UDEV
}

INCLUDEPATH += \
    ../../include \
    ../../src

HEADERS += \
    ../../include/serialport.h \
    ../../include/serialportinfo.h

SOURCES += main.cpp

include(../../src/src.pri)

CONFIG(debug, debug|release) {
    DESTDIR = debug
    TARGET = consoleinfod
} else {
    DESTDIR = release
    TARGET = consoleinfo
}

