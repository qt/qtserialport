# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(TARGET Qt::Widgets)
    qt_internal_add_example(terminal)
    qt_internal_add_example(blockingsender)
    qt_internal_add_example(blockingreceiver)
endif()
