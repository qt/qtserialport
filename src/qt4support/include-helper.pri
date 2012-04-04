system("mkdir -p $$PWD/../../include/$$QT.serialport.name")

for(header_file, PUBLIC_HEADERS) {
    system("cp $${header_file} $$PWD/../../include/$$QT.serialport.name")
}

header_files.files  = $$PUBLIC_HEADERS
header_files.path   = $$[QT_INSTALL_PREFIX]/include/$$QT.serialport.name
INSTALLS            += header_files
