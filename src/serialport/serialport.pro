QT = core
DEFINES += QT_SERIALPORT_LIB

include($$PWD/doc/doc.pri)
include($$PWD/serialport-lib.pri)

greaterThan(QT_MAJOR_VERSION, 4) {
    load(qt_build_config)
    QT += core-private
    TARGET = QtSerialPort
    load(qt_module)
} else {
    TEMPLATE = lib
    TARGET = $$qtLibraryTarget(QtSerialPort$$QT_LIBINFIX)
    include($$PWD/qt4support/install-helper.pri)
    CONFIG += module create_prl
    win32|mac:CONFIG += debug_and_release build_all
    mac:QMAKE_FRAMEWORK_BUNDLE_NAME = $$TARGET
}
