// Copyright (C) 2014 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

class tst_QSerialPortInfo : public QObject
{
    Q_OBJECT
public:
    explicit tst_QSerialPortInfo();

private slots:
    void initTestCase();

    void constructors();
    void assignment();

private:
    QString m_senderPortName;
    QString m_receiverPortName;
    QStringList m_availablePortNames;
};

tst_QSerialPortInfo::tst_QSerialPortInfo()
{
}

void tst_QSerialPortInfo::initTestCase()
{
    m_senderPortName = QString::fromLocal8Bit(qgetenv("QTEST_SERIALPORT_SENDER"));
    m_receiverPortName = QString::fromLocal8Bit(qgetenv("QTEST_SERIALPORT_RECEIVER"));
    if (m_senderPortName.isEmpty() || m_receiverPortName.isEmpty()) {
        static const char message[] =
              "Test doesn't work because the names of serial ports aren't found in env.\n"
              "Please set environment variables:\n"
              " QTEST_SERIALPORT_SENDER to name of output serial port\n"
              " QTEST_SERIALPORT_RECEIVER to name of input serial port\n"
              "Specify short names of port"
#if defined(Q_OS_UNIX)
              ", like 'ttyS0'\n";
#elif defined(Q_OS_WIN32)
              ", like 'COM1'\n";
#else
              "\n";
#endif
        QSKIP(message);
    } else {
        m_availablePortNames << m_senderPortName << m_receiverPortName;
    }
}

void tst_QSerialPortInfo::constructors()
{
    QSerialPortInfo empty;
    QVERIFY(empty.isNull());
    QSerialPortInfo empty2(QLatin1String("ABCD"));
    QVERIFY(empty2.isNull());
    QSerialPortInfo empty3(empty);
    QVERIFY(empty3.isNull());

    QSerialPortInfo exist(m_senderPortName);
    QVERIFY(!exist.isNull());
    QSerialPortInfo exist2(exist);
    QVERIFY(!exist2.isNull());
}

void tst_QSerialPortInfo::assignment()
{
    QSerialPortInfo empty;
    QVERIFY(empty.isNull());
    QSerialPortInfo empty2;
    empty2 = empty;
    QVERIFY(empty2.isNull());

    QSerialPortInfo exist(m_senderPortName);
    QVERIFY(!exist.isNull());
    QSerialPortInfo exist2;
    exist2 = exist;
    QVERIFY(!exist2.isNull());
}

QTEST_MAIN(tst_QSerialPortInfo)
#include "tst_qserialportinfo.moc"
