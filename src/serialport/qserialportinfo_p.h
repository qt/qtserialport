/****************************************************************************
**
** Copyright (C) 2011-2012 Denis Shienkov <denis.shienkov@gmail.com>
** Copyright (C) 2011 Sergey Belyashov <Sergey.Belyashov@gmail.com>
** Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtSerialPort module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QSERIALPORTINFO_P_H
#define QSERIALPORTINFO_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

class Q_AUTOTEST_EXPORT QSerialPortInfoPrivate
{
public:
    static QString portNameToSystemLocation(const QString &source);
    static QString portNameFromSystemLocation(const QString &source);

    QString portName;
    QString device;
    QString description;
    QString manufacturer;
    QString serialNumber;

    quint16 vendorIdentifier = 0;
    quint16 productIdentifier = 0;

    bool hasVendorIdentifier = false;
    bool hasProductIdentifier = false;
};

class QSerialPortInfoPrivateDeleter
{
public:
    static void cleanup(QSerialPortInfoPrivate *p) {
        delete p;
    }
};

QT_END_NAMESPACE

#endif // QSERIALPORTINFO_P_H
