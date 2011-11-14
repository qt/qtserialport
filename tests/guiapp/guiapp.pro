QT += core gui
TEMPLATE = app

unix {
    !macx:DEFINES += HAVE_UDEV
}

INCLUDEPATH += \
    ../../include \
    ../../src

HEADERS += \
    ../../include/serialport.h \
    ../../include/serialportinfo.h

include(../../src/src.pri)

SOURCES += main.cpp\
    maindialog.cpp \
    optionsdialog.cpp \
    tracedialog.cpp
HEADERS += maindialog.h \
    optionsdialog.h \
    tracedialog.h
FORMS += maindialog.ui \
    optionsdialog.ui \
    tracedialog.ui

CONFIG(debug, debug|release) {
    DESTDIR = debug
    TARGET = guiappd
} else {
    DESTDIR = release
    TARGET = guiapp
}
