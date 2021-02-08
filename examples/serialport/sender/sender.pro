QT += widgets serialport
requires(qtConfig(combobox))

TARGET = sender
TEMPLATE = app

HEADERS += \
    dialog.h

SOURCES += \
    main.cpp \
    dialog.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/serialport/sender
INSTALLS += target
