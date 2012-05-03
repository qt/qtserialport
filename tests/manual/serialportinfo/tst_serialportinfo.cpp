/****************************************************************************
**
** Copyright (C) 2012 Denis Shienkov <scapig2@yandex.ru>
** Contact: http://www.qt-project.org/
**
** This file is part of the QtSerialPort module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QtCore/QDebug>

#include <QtAddOnSerialPort/serialportinfo.h>
#include <QtAddOnSerialPort/serialport.h>

QT_USE_NAMESPACE_SERIALPORT

class tst_SerialPortInfo : public QObject
{
    Q_OBJECT

private slots:
    void ports();
    void constructors();
    void assignment();
};

void tst_SerialPortInfo::ports()
{
    QList<SerialPortInfo> list(SerialPortInfo::availablePorts());
    QCOMPARE(list.isEmpty(), false);
}

void tst_SerialPortInfo::constructors()
{
    // FIXME
}

void tst_SerialPortInfo::assignment()
{
    QList<SerialPortInfo> list(SerialPortInfo::availablePorts());

    for (int c = 0; c < list.size(); ++c) {
        SerialPortInfo info = list[c];
        QCOMPARE(info.portName(), list[c].portName());
        QCOMPARE(info.systemLocation(), list[c].systemLocation());
        QCOMPARE(info.description(), list[c].description());
        QCOMPARE(info.manufacturer(), list[c].manufacturer());
        QCOMPARE(info.vendorIdentifier(), list[c].vendorIdentifier());
        QCOMPARE(info.productIdentifier(), list[c].productIdentifier());
    }
}

QTEST_MAIN(tst_SerialPortInfo)
#include "tst_serialportinfo.moc"
