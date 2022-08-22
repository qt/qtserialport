# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qserialportinfoprivate Binary:
#####################################################################

qt_internal_add_test(tst_qserialportinfoprivate
    SOURCES
        tst_qserialportinfoprivate.cpp
    LIBRARIES
        Qt::SerialPortPrivate
        Qt::Test
)
