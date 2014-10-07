QT = core testlib
TARGET = tst_qserialport
#CONFIG += testcase

include($$QTSERIALPORT_PROJECT_ROOT/src/serialport/qt4support/serialport.prf)

SOURCES = tst_qserialport.cpp
