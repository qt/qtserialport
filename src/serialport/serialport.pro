TARGET = QtSerialPort
QT = core-private

QMAKE_DOCS = $$PWD/doc/qtserialport.qdocconf

load(qt_module)

include($$PWD/serialport-lib.pri)

PRECOMPILED_HEADER =
