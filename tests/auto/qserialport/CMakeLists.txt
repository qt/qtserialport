# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qserialport Binary:
#####################################################################

qt_internal_add_test(tst_qserialport
    SOURCES
        tst_qserialport.cpp
    LIBRARIES
        Qt::SerialPort
        Qt::Test
        Qt::TestPrivate
)
