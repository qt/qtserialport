QT.serialport.VERSION = 1.0.0
QT.serialport.MAJOR_VERSION = 1
QT.serialport.MINOR_VERSION = 0
QT.serialport.PATCH_VERSION = 0

QT.serialport.name = SerialPort
QT.serialport.bins = $$QT_MODULE_BIN_BASE
QT.serialport.includes = $$QT_MODULE_INCLUDE_BASE $$QT_MODULE_INCLUDE_BASE/QtAddOnSerialPort
QT.serialport.private_includes = $$QT_MODULE_INCLUDE_BASE/$$QT.serialport.name/$$QT.serialport.VERSION
QT.serialport.sources = $$QT_MODULE_BASE/src
QT.serialport.libs = $$QT_MODULE_LIB_BASE
QT.serialport.plugins = $$QT_MODULE_PLUGIN_BASE
QT.serialport.imports = $$QT_MODULE_IMPORT_BASE
QT.serialport.depends = core
