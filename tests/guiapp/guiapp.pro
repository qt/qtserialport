QT += core gui
TEMPLATE = app

INCLUDEPATH += \
    ../../include \
    ../../src

HEADERS += \
    ../../include/serialport.h \
    ../../include/serialportinfo.h

include(../../src/src.pri)

SOURCES += main.cpp\
    mainwidget.cpp \
    optionswidget.cpp \
    tracewidget.cpp
HEADERS += mainwidget.h \
    optionswidget.h \
    tracewidget.h
FORMS += mainwidget.ui \
    optionswidget.ui \
    tracewidget.ui

CONFIG(debug, debug|release) {
    DESTDIR = debug
    TARGET = guiappd
} else {
    DESTDIR = release
    TARGET = guiapp
}

win32 {
    LIBS += -lsetupapi -luuid -ladvapi32
}
unix:!macx {
    #LIBS += -ludev
}
