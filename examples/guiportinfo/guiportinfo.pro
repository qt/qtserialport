#-------------------------------------------------
#
# Project created by QtCreator 2011-09-21T13:44:17
#
#-------------------------------------------------

QT       += core gui

TARGET = guiportinfo
TEMPLATE = app

unix {
    !macx:DEFINES += HAVE_UDEV
}

INCLUDEPATH += \
    ../../include 

HEADERS += \
    ../../include/serialport.h \
    ../../include/serialportinfo.h

include(../../src/src.pri)

SOURCES += main.cpp\
        portinfodialog.cpp

HEADERS  += portinfodialog.h

FORMS    += portinfodialog.ui
