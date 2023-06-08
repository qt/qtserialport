// Copyright (C) 2014 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QtTest>

#include <private/qserialportinfo_p.h>

class tst_QSerialPortInfoPrivate : public QObject
{
    Q_OBJECT
public:
    explicit tst_QSerialPortInfoPrivate();

private slots:
    void canonical_data();
    void canonical();
};

tst_QSerialPortInfoPrivate::tst_QSerialPortInfoPrivate()
{
}

void tst_QSerialPortInfoPrivate::canonical_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("location");

#if defined(Q_OS_WINCE)
    QTest::newRow("Test1") << "COM1" << "COM1" << "COM1:";
    QTest::newRow("Test2") << "COM1:" << "COM1" << "COM1:";
#elif defined(Q_OS_WIN32)
    QTest::newRow("Test1") << "COM1" << "COM1" << "\\\\.\\COM1";
    QTest::newRow("Test2") << "\\\\.\\COM1" << "COM1" << "\\\\.\\COM1";
    QTest::newRow("Test3") << "//./COM1" << "COM1" << "//./COM1";
#elif defined(Q_OS_MACOS)
    QTest::newRow("Test1") << "ttyS0" << "ttyS0" << "/dev/ttyS0";
    QTest::newRow("Test2") << "cu.serial1" << "cu.serial1" << "/dev/cu.serial1";
    QTest::newRow("Test3") << "tty.serial1" << "tty.serial1" << "/dev/tty.serial1";
    QTest::newRow("Test4") << "/dev/ttyS0" << "ttyS0" << "/dev/ttyS0";
    QTest::newRow("Test5") << "/dev/tty.serial1" << "tty.serial1" << "/dev/tty.serial1";
    QTest::newRow("Test6") << "/dev/cu.serial1" << "cu.serial1" << "/dev/cu.serial1";
    QTest::newRow("Test7") << "/dev/serial/ttyS0" << "serial/ttyS0" << "/dev/serial/ttyS0";
    QTest::newRow("Test8") << "/home/ttyS0" << "/home/ttyS0" << "/home/ttyS0";
    QTest::newRow("Test9") << "/home/serial/ttyS0" << "/home/serial/ttyS0" << "/home/serial/ttyS0";
    QTest::newRow("Test10") << "serial/ttyS0" << "serial/ttyS0" << "/dev/serial/ttyS0";
    QTest::newRow("Test11") << "./ttyS0" << "./ttyS0" << "./ttyS0";
    QTest::newRow("Test12") << "../ttyS0" << "../ttyS0" << "../ttyS0";
#elif defined(Q_OS_UNIX)
    QTest::newRow("Test1") << "ttyS0" << "ttyS0" << "/dev/ttyS0";
    QTest::newRow("Test2") << "/dev/ttyS0" << "ttyS0" << "/dev/ttyS0";
    QTest::newRow("Test3") << "/dev/serial/ttyS0" << "serial/ttyS0" << "/dev/serial/ttyS0";
    QTest::newRow("Test4") << "/home/ttyS0" << "/home/ttyS0" << "/home/ttyS0";
    QTest::newRow("Test5") << "/home/serial/ttyS0" << "/home/serial/ttyS0" << "/home/serial/ttyS0";
    QTest::newRow("Test6") << "serial/ttyS0" << "serial/ttyS0" << "/dev/serial/ttyS0";
    QTest::newRow("Test7") << "./ttyS0" << "./ttyS0" << "./ttyS0";
    QTest::newRow("Test8") << "../ttyS0" << "../ttyS0" << "../ttyS0";
#endif
}

void tst_QSerialPortInfoPrivate::canonical()
{
    QFETCH(QString, source);
    QFETCH(QString, name);
    QFETCH(QString, location);

    QCOMPARE(QSerialPortInfoPrivate::portNameFromSystemLocation(source), name);
    QCOMPARE(QSerialPortInfoPrivate::portNameToSystemLocation(source), location);
}

QTEST_MAIN(tst_QSerialPortInfoPrivate)
#include "tst_qserialportinfoprivate.moc"
