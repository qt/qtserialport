TEMPLATE = subdirs
SUBDIRS = src examples #tests
CONFIG += ordered
include(doc/doc.pri)

lessThan(QT_MAJOR_VERSION, 5) {
    !infile(.qmake.cache, SERIALPORT_PROJECT_ROOT):system("echo \"SERIALPORT_PROJECT_ROOT = $$OUT_PWD\" >> .qmake.cache")
}
