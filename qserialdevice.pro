TEMPLATE = lib
CONFIG += dll
QT -= gui
OBJECTS_DIR = build/obj
MOC_DIR = build/moc

win32:DEFINES += QSERIALDEVICE_SHARED

INCLUDEPATH += src include

include(src/src.pri)

CONFIG(debug, debug|release) {
    DESTDIR = build/debug
    TARGET = qserialdeviced
} else {
    DESTDIR = build/release
    TARGET = qserialdevice
}

win32 {
    LIBS += -lsetupapi -luuid -ladvapi32
}
#unix:!macx {
#    LIBS += -ludev
#}
macx {
    LIBS += -framework IOKit -framework CoreFoundation
}
