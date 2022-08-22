# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qserialportinfo Binary:
#####################################################################

qt_internal_add_test(tst_qserialportinfo
    SOURCES
        tst_qserialportinfo.cpp
    LIBRARIES
        Qt::SerialPort
        Qt::Test
)
