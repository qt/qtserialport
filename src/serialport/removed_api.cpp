// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#define QT_SERIALPORT_BUILD_REMOVED_API

#include "qserialportglobal.h"

QT_USE_NAMESPACE

#if QT_SERIALPORT_REMOVED_SINCE(6, 7)

#include "qserialport.h"
#include "qserialport_p.h"

QBindable<bool> QSerialPort::bindableStopBits()
{
    return &d_func()->stopBits;
}

#endif // QT_SERIALPORT_REMOVED_SINCE(6, 7)
