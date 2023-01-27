# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_example(creaderasync)
qt_internal_add_example(cwriterasync)
if((((NOT QT.widgets.name_ISEMPTY))))
    qt_internal_add_example(terminal)
    qt_internal_add_example(blockingsender)
    qt_internal_add_example(blockingreceiver)
    qt_internal_add_example(sender)
    qt_internal_add_example(receiver)
endif()
