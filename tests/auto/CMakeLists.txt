# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

add_subdirectory(qserialport)
add_subdirectory(qserialportinfo)
add_subdirectory(cmake)
if(QT_FEATURE_private_tests)
    add_subdirectory(qserialportinfoprivate)
endif()
