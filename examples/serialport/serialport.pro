TEMPLATE = subdirs
SUBDIRS = cenumerator creaderasync creadersync cwriterasync cwritersync
!isEmpty(QT.widgets.name):SUBDIRS += enumerator terminal blockingsender blockingreceiver sender receiver
