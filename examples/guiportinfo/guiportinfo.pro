#-------------------------------------------------
#
# Project created by QtCreator 2011-09-21T13:44:17
#
#-------------------------------------------------

QT       += core gui

TARGET = guiportinfo
TEMPLATE = app

linux*:DEFINES += HAVE_LIBUDEV

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
