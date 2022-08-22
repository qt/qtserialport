# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(qserialport)
add_subdirectory(qserialportinfo)
add_subdirectory(cmake)
if(QT_FEATURE_private_tests)
    add_subdirectory(qserialportinfoprivate)
endif()
