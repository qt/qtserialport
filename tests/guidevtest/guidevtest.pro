#-------------------------------------------------
#
# Project created by QtCreator 2011-10-15T16:26:56
#
#-------------------------------------------------

QT       += core gui

TARGET = guidevtest
TEMPLATE = app

INCLUDEPATH += \
    ../../include \
    ../../src

HEADERS += \
    ../../include/serialport.h \
    ../../include/serialportinfo.h

include(../../src/src.pri)

SOURCES += \
    main.cpp \
    maindialog.cpp \
    unittestinfo.cpp \
    unittestsignals.cpp \
    unittestwaitforx.cpp

HEADERS += \
    maindialog.h \
    unittests.h

FORMS += \
    maindialog.ui










