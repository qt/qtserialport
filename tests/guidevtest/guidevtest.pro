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
    ../../include/serialportinfo.h \
    unittestmanager.h

include(../../src/src.pri)

SOURCES += \
    main.cpp \
    maindialog.cpp \
    testsdialog.cpp \
    unittestmanager.cpp

HEADERS += \
    maindialog.h \
    testsdialog.h

FORMS += \
    maindialog.ui \
    testsdialog.ui





