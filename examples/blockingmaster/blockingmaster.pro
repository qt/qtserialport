greaterThan(QT_MAJOR_VERSION, 4) {
    QT       += widgets serialport
} else {
    include($$SERIALPORT_PROJECT_ROOT/src/serialport/qt4support/serialport.prf)
}

TARGET = blockingmaster
TEMPLATE = app

HEADERS += \
    blockingmasterwidget.h \
    transactionthread.h

SOURCES += \
    main.cpp \
    blockingmasterwidget.cpp \
    transactionthread.cpp
