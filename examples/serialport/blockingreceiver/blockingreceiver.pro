QT += widgets serialport
requires(qtConfig(combobox))

TARGET = blockingreceiver
TEMPLATE = app

HEADERS += \
    dialog.h \
    receiverthread.h

SOURCES += \
    main.cpp \
    dialog.cpp \
    receiverthread.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/serialport/blockingreceiver
INSTALLS += target
