QT += widgets serialport

TARGET = blockingmaster
TEMPLATE = app

HEADERS += \
    dialog.h \
    masterthread.h

SOURCES += \
    main.cpp \
    dialog.cpp \
    masterthread.cpp
