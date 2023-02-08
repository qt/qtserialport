QT += widgets serialport
requires(qtConfig(combobox))

TARGET = receiver
TEMPLATE = app

HEADERS += \
    dialog.h

SOURCES += \
    main.cpp \
    dialog.cpp
