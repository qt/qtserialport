TEMPLATE = subdirs
SUBDIRS = creaderasync cwriterasync
!isEmpty(QT.widgets.name):SUBDIRS += terminal blockingsender blockingreceiver
