include($$QTSERIALPORT_PROJECT_ROOT/src/serialport/qt4support/serialport.prf)

TARGET = slave
TEMPLATE = app

HEADERS += \
    dialog.h

SOURCES += \
    main.cpp \
    dialog.cpp
