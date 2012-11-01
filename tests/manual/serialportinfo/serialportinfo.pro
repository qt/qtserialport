CONFIG += testcase
TARGET = tst_serialportinfo

QT = core testlib
greaterThan(QT_MAJOR_VERSION, 4) {
    QT += serialport
} else {
    include($$SERIALPORT_PROJECT_ROOT/src/serialport/qt4support/serialport.prf)
}

SOURCES += tst_serialportinfo.cpp
