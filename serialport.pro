TEMPLATE = subdirs
SUBDIRS = src examples #tests
CONFIG += ordered
include(doc/doc.pri)

lessThan(QT_MAJOR_VERSION, 5) {
    !infile($$OUT_PWD/.qmake.cache, SERIALPORT_PROJECT_ROOT) {
        system("echo \"SERIALPORT_PROJECT_ROOT = $$PWD\" >> $$OUT_PWD/.qmake.cache")
        system("echo \"SERIALPORT_BUILD_ROOT = $$OUT_PWD\" >> $$OUT_PWD/.qmake.cache")
    }
}
