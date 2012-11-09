greaterThan(QT_MAJOR_VERSION, 4) {
    QT       += widgets serialport
} else {
    include($$SERIALPORT_PROJECT_ROOT/src/serialport/qt4support/serialport.prf)
}

TARGET = blockingslave
TEMPLATE = app

HEADERS += \
    blockingslavewidget.h \
    transactionthread.h

SOURCES += \
    main.cpp \
    blockingslavewidget.cpp \
    transactionthread.cpp
