TEMPLATE = lib
MODULE   = serialport

greaterThan(QT_MAJOR_VERSION, 4) {
    TARGET   = $$QT.serialport.name
    load(qt_module)
    load(qt_module_config)
    QT += core-private
    include($$PWD/src-lib.pri)
} else {
    include($$PWD/qt4support/serialport.pri)
    TARGET   = $$qtLibraryTarget($$QT.serialport.name)
    include($$PWD/src-lib.pri)
    include($$PWD/qt4support/include-helper.pri)
}

DESTDIR = $$QT.serialport.libs
VERSION = $$QT.serialport.VERSION
DEFINES += QT_ADDON_SERIALPORT_LIB

CONFIG += module create_prl
MODULE_PRI = ../modules/qt_serialport.pri

mac:QMAKE_FRAMEWORK_BUNDLE_NAME = $$QT.serialport.name


