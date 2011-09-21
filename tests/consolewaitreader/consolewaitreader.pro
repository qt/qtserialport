TEMPLATE = app
CONFIG += console
QT -= gui
OBJECTS_DIR = obj
MOC_DIR = moc

INCLUDEPATH += \
    ../../include \
    ../../src

HEADERS += \
    ../../include/serialport.h \
    ../../include/serialportinfo.h

include(../../src/src.pri)

SOURCES += main.cpp

CONFIG(debug, debug|release) {
    DESTDIR = debug
    TARGET = consolewaitreaderd
} else {
    DESTDIR = release
    TARGET = consolewaitreader
}
