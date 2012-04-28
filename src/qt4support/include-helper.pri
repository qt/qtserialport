SERIALPORT_PROJECT_INCLUDEDIR = $$SERIALPORT_BUILD_ROOT/include/QtAddOnSerialPort
SERIALPORT_PROJECT_INCLUDEDIR ~=s,/,$$QMAKE_DIR_SEP,

system("$$QMAKE_MKDIR $$SERIALPORT_PROJECT_INCLUDEDIR")

for(header_file, PUBLIC_HEADERS) {
   header_file ~=s,/,$$QMAKE_DIR_SEP,
   system("$$QMAKE_COPY $${header_file} $$SERIALPORT_PROJECT_INCLUDEDIR")
}

header_files.files  = $$PUBLIC_HEADERS
header_files.path   = $$[QT_INSTALL_PREFIX]/include/QtAddOnSerialPort
INSTALLS            += header_files

serialport_prf.files    = serialport.prf
serialport_prf.path     = $$[QT_INSTALL_DATA]/mkspecs/features
INSTALLS                += serialport_prf
