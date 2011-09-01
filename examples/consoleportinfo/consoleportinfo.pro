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

SOURCES += main.cpp

include(../../src/src.pri)

CONFIG(debug, debug|release) {
    DESTDIR = debug
    TARGET = consoleinfod
} else {
    DESTDIR = release
    TARGET = consoleinfo
}

win32 {
    !wince*: LIBS += -lsetupapi -luuid -ladvapi32
}
unix:!macx {
    LIBS += -ludev
}
macx {
    LIBS += -framework IOKit -framework CoreFoundation
}
