# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#####################################################################
## tst_qserialportinfoprivate Binary:
#####################################################################

qt_internal_add_test(tst_qserialportinfoprivate
    SOURCES
        tst_qserialportinfoprivate.cpp
    PUBLIC_LIBRARIES
        Qt::SerialPortPrivate
        Qt::Test
)
