system("mkdir -p $$PWD/../../include/$$QT.serialport.name")

for(header_file, PUBLIC_HEADERS) {
    system("cp $${header_file} $$PWD/../../include/$$QT.serialport.name")
}

header_files.files  = $$PUBLIC_HEADERS
header_files.path   = $$[QT_INSTALL_PREFIX]/include/$$QT.serialport.name
INSTALLS            += header_files

serialport_prf.files    = serialport.prf
serialport_prf.path     = $$[QT_INSTALL_DATA]/mkspecs/features
INSTALLS                += serialport_prf
