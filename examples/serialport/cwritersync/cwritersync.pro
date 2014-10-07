QT = core

include($$QTSERIALPORT_PROJECT_ROOT/src/serialport/qt4support/serialport.prf)

CONFIG += console
CONFIG -= app_bundle

TARGET = cwritersync
TEMPLATE = app

SOURCES += \
    main.cpp
