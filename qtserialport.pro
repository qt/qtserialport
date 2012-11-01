include(doc/doc.pri)

lessThan(QT_MAJOR_VERSION, 5) {
    TEMPLATE = subdirs
    SUBDIRS = src examples tests
    CONFIG += ordered

    !infile($$OUT_PWD/.qmake.cache, SERIALPORT_PROJECT_ROOT) {
        system("echo SERIALPORT_PROJECT_ROOT = $$PWD >> $$OUT_PWD/.qmake.cache")
        system("echo SERIALPORT_BUILD_ROOT = $$OUT_PWD >> $$OUT_PWD/.qmake.cache")
    }
} else {
    load(qt_parts)
}
