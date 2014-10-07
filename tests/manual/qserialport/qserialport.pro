TEMPLATE = app
TARGET = tst_qserialport

QT = core testlib

include($$QTSERIALPORT_PROJECT_ROOT/src/serialport/qt4support/serialport.prf)

SOURCES += tst_qserialport.cpp
