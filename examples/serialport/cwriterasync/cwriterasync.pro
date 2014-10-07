QT = core

include($$QTSERIALPORT_PROJECT_ROOT/src/serialport/qt4support/serialport.prf)

CONFIG += console
CONFIG -= app_bundle

TARGET = cwriterasync
TEMPLATE = app

HEADERS += \
    serialportwriter.h

SOURCES += \
    main.cpp \
    serialportwriter.cpp
