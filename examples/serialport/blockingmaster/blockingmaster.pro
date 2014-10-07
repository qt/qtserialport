include($$QTSERIALPORT_PROJECT_ROOT/src/serialport/qt4support/serialport.prf)

TARGET = blockingmaster
TEMPLATE = app

HEADERS += \
    dialog.h \
    masterthread.h

SOURCES += \
    main.cpp \
    dialog.cpp \
    masterthread.cpp
