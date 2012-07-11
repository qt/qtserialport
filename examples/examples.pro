TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = cenumerator
greaterThan(QT_MAJOR_VERSION, 4) {
    !isEmpty(QT.widgets.name):SUBDIRS += enumerator terminal
} else {
    SUBDIRS += enumerator terminal
}
