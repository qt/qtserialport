/****************************************************************************
**
** Copyright (C) 2012 Denis Shienkov <denis.shienkov@gmail.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtSerialPort module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QtCore/QDebug>

#include <QtSerialPort/qserialportinfo.h>
#include <QtSerialPort/qserialport.h>

QT_USE_NAMESPACE_SERIALPORT

class tst_QSerialPortInfo : public QObject
{
    Q_OBJECT

private slots:
    void serialPortInfoList();
    void constructors();
    void assignment();
};

void tst_QSerialPortInfo::serialPortInfoList()
{
    QList<QSerialPortInfo> list(QSerialPortInfo::availablePorts());
    QCOMPARE(list.isEmpty(), false);
}

void tst_QSerialPortInfo::constructors()
{
    // FIXME
}

void tst_QSerialPortInfo::assignment()
{
    QList<QSerialPortInfo> list(QSerialPortInfo::availablePorts());

    for (int c = 0; c < list.size(); ++c) {
        QSerialPortInfo info = list[c];
        QCOMPARE(info.portName(), list[c].portName());
        QCOMPARE(info.systemLocation(), list[c].systemLocation());
        QCOMPARE(info.description(), list[c].description());
        QCOMPARE(info.manufacturer(), list[c].manufacturer());
        QCOMPARE(info.vendorIdentifier(), list[c].vendorIdentifier());
        QCOMPARE(info.productIdentifier(), list[c].productIdentifier());
    }
}

QTEST_MAIN(tst_QSerialPortInfo)
#include "tst_qserialportinfo.moc"
