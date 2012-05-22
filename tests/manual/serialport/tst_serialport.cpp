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

class tst_SerialPort : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void open();
    void rate();
    void dataBits();
    void parity();
    void stopBits();
    void flowControl();

private:
    QList<SerialPortInfo> ports;
};

void tst_SerialPort::initTestCase()
{
    ports = SerialPortInfo::availablePorts();

    if (ports.isEmpty()) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        QSKIP("Test doesn't work because the serial ports are not detected.");
#else
        QSKIP("Test doesn't work because the serial ports are not detected.", SkipAll);
#endif
    }
}

void tst_SerialPort::open()
{
    foreach (const SerialPortInfo &info, ports) {

        if (info.isBusy())
            continue;

        SerialPort object1;

        // Try open and check access to port by Info
        object1.setPort(info);
        QCOMPARE(object1.portName(), info.portName());
        QCOMPARE(object1.open(QIODevice::ReadWrite), true);
        QCOMPARE(object1.isOpen(), true);
        object1.close();
        QCOMPARE(object1.isOpen(), false);

        // Try open and check access to port by Name
        object1.setPort(info.portName());
        QCOMPARE(object1.portName(), info.portName());
        QCOMPARE(object1.open(QIODevice::ReadWrite), true);
        QCOMPARE(object1.isOpen(), true);
        object1.close();
        QCOMPARE(object1.isOpen(), false);

        // Try open and check access to port by Location
        object1.setPort(info.systemLocation());
        QCOMPARE(object1.portName(), info.portName());
        QCOMPARE(object1.open(QIODevice::ReadWrite), true);
        QCOMPARE(object1.isOpen(), true);
        object1.close();
        QCOMPARE(object1.isOpen(), false);
    }
}

void tst_SerialPort::rate()
{
    foreach (const SerialPortInfo &info, ports) {

        SerialPort object1;
        object1.setPort(info.portName());
        QCOMPARE(object1.open(QIODevice::ReadWrite), true);

        QCOMPARE(object1.setRate(SerialPort::Rate1200), true);
        QCOMPARE(object1.rate(), static_cast<qint32>(SerialPort::Rate1200));
        QCOMPARE(object1.setRate(SerialPort::Rate2400), true);
        QCOMPARE(object1.rate(), static_cast<qint32>(SerialPort::Rate2400));
        QCOMPARE(object1.setRate(SerialPort::Rate4800), true);
        QCOMPARE(object1.rate(), static_cast<qint32>(SerialPort::Rate4800));
        QCOMPARE(object1.setRate(SerialPort::Rate9600), true);
        QCOMPARE(object1.rate(), static_cast<qint32>(SerialPort::Rate9600));
        QCOMPARE(object1.setRate(SerialPort::Rate19200), true);
        QCOMPARE(object1.rate(), static_cast<qint32>(SerialPort::Rate19200));
        QCOMPARE(object1.setRate(SerialPort::Rate38400), true);
        QCOMPARE(object1.rate(), static_cast<qint32>(SerialPort::Rate38400));
        QCOMPARE(object1.setRate(SerialPort::Rate57600), true);
        QCOMPARE(object1.rate(), static_cast<qint32>(SerialPort::Rate57600));
        QCOMPARE(object1.setRate(SerialPort::Rate115200), true);
        QCOMPARE(object1.rate(), static_cast<qint32>(SerialPort::Rate115200));

        object1.close();
    }
}

void tst_SerialPort::dataBits()
{
    foreach (const SerialPortInfo &info, ports) {

        SerialPort object1;
        object1.setPort(info.portName());
        QCOMPARE(object1.open(QIODevice::ReadWrite), true);

        QCOMPARE(object1.setDataBits(SerialPort::Data8), true);
        QCOMPARE(object1.dataBits(), SerialPort::Data8);

        object1.close();
    }
}

void tst_SerialPort::parity()
{
    foreach (const SerialPortInfo &info, ports) {

        SerialPort object1;
        object1.setPort(info.portName());
        QCOMPARE(object1.open(QIODevice::ReadWrite), true);

        QCOMPARE(object1.setParity(SerialPort::NoParity), true);
        QCOMPARE(object1.parity(), SerialPort::NoParity);
        QCOMPARE(object1.setParity(SerialPort::EvenParity), true);
        QCOMPARE(object1.parity(), SerialPort::EvenParity);
        QCOMPARE(object1.setParity(SerialPort::OddParity), true);
        QCOMPARE(object1.parity(), SerialPort::OddParity);
        QCOMPARE(object1.setParity(SerialPort::MarkParity), true);
        QCOMPARE(object1.parity(), SerialPort::MarkParity);
        QCOMPARE(object1.setParity(SerialPort::SpaceParity), true);
        QCOMPARE(object1.parity(), SerialPort::SpaceParity);

        object1.close();
    }
}

void tst_SerialPort::stopBits()
{
    foreach (const SerialPortInfo &info, ports) {

        SerialPort object1;
        object1.setPort(info.portName());
        QCOMPARE(object1.open(QIODevice::ReadWrite), true);

        QCOMPARE(object1.setStopBits(SerialPort::OneStop), true);
        QCOMPARE(object1.stopBits(), SerialPort::OneStop);
        // skip 1.5 stop bits
        QCOMPARE(object1.setStopBits(SerialPort::TwoStop), true);
        QCOMPARE(object1.stopBits(), SerialPort::TwoStop);

        object1.close();
    }
}

void tst_SerialPort::flowControl()
{
    foreach (const SerialPortInfo &info, ports) {

        SerialPort object1;
        object1.setPort(info.portName());
        QCOMPARE(object1.open(QIODevice::ReadWrite), true);

        QCOMPARE(object1.setFlowControl(SerialPort::NoFlowControl), true);
        QCOMPARE(object1.flowControl(), SerialPort::NoFlowControl);
        QCOMPARE(object1.setFlowControl(SerialPort::HardwareControl), true);
        QCOMPARE(object1.flowControl(), SerialPort::HardwareControl);
        QCOMPARE(object1.setFlowControl(SerialPort::SoftwareControl), true);
        QCOMPARE(object1.flowControl(), SerialPort::SoftwareControl);

        object1.close();
    }
}

QTEST_MAIN(tst_SerialPort)
#include "tst_serialport.moc"
