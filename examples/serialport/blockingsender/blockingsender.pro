QT += widgets serialport
requires(qtConfig(combobox))

TARGET = blockingsender
TEMPLATE = app

HEADERS += \
    dialog.h \
    senderthread.h

SOURCES += \
    main.cpp \
    dialog.cpp \
    senderthread.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/serialport/blockingsender
INSTALLS += target
