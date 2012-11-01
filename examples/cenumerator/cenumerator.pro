greaterThan(QT_MAJOR_VERSION, 4) {
    QT       += core serialport
} else {
    include($$SERIALPORT_PROJECT_ROOT/src/serialport/qt4support/serialport.prf)
}

TARGET = cenumerator
TEMPLATE = app

SOURCES += \
    main.cpp
