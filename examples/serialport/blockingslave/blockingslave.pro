QT += widgets serialport

TARGET = blockingslave
TEMPLATE = app

HEADERS += \
    dialog.h \
    slavethread.h

SOURCES += \
    main.cpp \
    dialog.cpp \
    slavethread.cpp
