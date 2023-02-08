QT += widgets serialport
requires(qtConfig(combobox))

TARGET = sender
TEMPLATE = app

HEADERS += \
    dialog.h

SOURCES += \
    main.cpp \
    dialog.cpp
