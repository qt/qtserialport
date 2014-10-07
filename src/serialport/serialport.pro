QT = core

QMAKE_DOCS = $$PWD/doc/qtserialport.qdocconf
include($$PWD/serialport-lib.pri)

TEMPLATE = lib
TARGET = $$qtLibraryTarget(QtSerialPort$$QT_LIBINFIX)
include($$PWD/qt4support/install-helper.pri)
CONFIG += module create_prl
mac:QMAKE_FRAMEWORK_BUNDLE_NAME = $$TARGET

PRECOMPILED_HEADER =
