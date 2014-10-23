QT = core
QT += serialport

CONFIG += console
CONFIG -= app_bundle

TARGET = creaderasync
TEMPLATE = app

HEADERS += \
    serialportreader.h

SOURCES += \
    main.cpp \
    serialportreader.cpp
