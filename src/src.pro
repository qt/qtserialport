greaterThan(QT_MAJOR_VERSION, 4) {
    load(qt_module)
    load(qt_module_config)
    QT += core-private
} else {
    include($$PWD/../modules/qt_serialport.pri)
}

TEMPLATE = lib
TARGET   = $$QT.serialport.name
MODULE   = serialport

DESTDIR = $$QT.serialport.libs
VERSION = $$QT.serialport.VERSION
DEFINES += QT_ADDON_SERIALPORT_LIB

CONFIG += module create_prl
MODULE_PRI = ../modules/qt_serialport.pri

include($$PWD/src-lib.pri)

mac:QMAKE_FRAMEWORK_BUNDLE_NAME = $$QT.serialport.name


