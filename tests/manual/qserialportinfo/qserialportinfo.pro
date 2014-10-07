TEMPLATE = app
TARGET = tst_qserialportinfo

QT = core testlib

include($$QTSERIALPORT_PROJECT_ROOT/src/serialport/qt4support/serialport.prf)

SOURCES += tst_qserialportinfo.cpp
