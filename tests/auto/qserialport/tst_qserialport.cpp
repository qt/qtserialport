/****************************************************************************
**
** Copyright (C) 2014 Denis Shienkov <denis.shienkov@gmail.com>
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
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

Q_DECLARE_METATYPE(QSerialPort::SerialPortError);
Q_DECLARE_METATYPE(QIODevice::OpenMode);
Q_DECLARE_METATYPE(QIODevice::OpenModeFlag);

class tst_QSerialPort : public QObject
{
    Q_OBJECT
public:
    explicit tst_QSerialPort();

    static void enterLoop(int secs)
    {
        ++loopLevel;
        QTestEventLoop::instance().enterLoop(secs);
        --loopLevel;
    }

    static void enterLoopMsecs(int msecs)
    {
        ++loopLevel;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        QTestEventLoop::instance().enterLoopMSecs(msecs);
#else
        Q_UNUSED(msecs);
        QTestEventLoop::instance().enterLoop(1);
#endif
        --loopLevel;
    }

    static void exitLoop()
    {
        if (loopLevel > 0)
            QTestEventLoop::instance().exitLoop();
    }

    static bool timeout()
    {
        return QTestEventLoop::instance().timeout();
    }

private slots:
    void initTestCase();

    void defaultConstruct();
    void constructByName();
    void constructByInfo();

    void openExisting_data();
    void openExisting();
    void openNotExisting_data();
    void openNotExisting();

    void flush();
    void doubleFlush();

    void waitForBytesWritten();

    void waitForReadyReadWithTimeout();
    void waitForReadyReadWithOneByte();
    void waitForReadyReadWithAlphabet();

protected slots:
    void handleBytesWrittenAndExitLoopSlot(qint64 bytesWritten);
    void handleBytesWrittenAndExitLoopSlot2(qint64 bytesWritten);

private:
#ifdef Q_OS_WIN
    void clearReceiver();
#endif

    QString m_senderPortName;
    QString m_receiverPortName;
    QStringList m_availablePortNames;

    static int loopLevel;
    static const QByteArray alphabetArray;
    static const QByteArray newlineArray;
};

int tst_QSerialPort::loopLevel = 0;

const QByteArray tst_QSerialPort::alphabetArray("ABCDEFGHIJKLMNOPQRSTUVWXUZ");
const QByteArray tst_QSerialPort::newlineArray("\n\r");

tst_QSerialPort::tst_QSerialPort()
{
}

#ifdef Q_OS_WIN
// This method is a workaround for the "com0com" virtual serial port
// driver, which is installed on CI. The problem is that the close/clear
// methods have no effect on sender serial port. If any data didn't manage
// to be transferred before closing, then this data will continue to be
// transferred at next opening of sender port.
// Thus, this behavior influences other tests and leads to the wrong results
// (e.g. the receiver port on other test can receive some data which are
// not expected). It is recommended to use this method for cleaning of
// read FIFO of receiver for those tests in which reception of data is
// required.
void tst_QSerialPort::clearReceiver()
{
    QSerialPort receiver(m_receiverPortName);
    if (receiver.open(QIODevice::ReadOnly))
        enterLoopMsecs(100);
}
#endif

void tst_QSerialPort::initTestCase()
{
    m_senderPortName = QString::fromLocal8Bit(qgetenv("QTEST_SERIALPORT_SENDER"));
    m_receiverPortName = QString::fromLocal8Bit(qgetenv("QTEST_SERIALPORT_RECEIVER"));
    if (m_senderPortName.isEmpty() || m_receiverPortName.isEmpty()) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        QSKIP("Test doesn't work because the names of serial ports aren't found in env.");
#else
        QSKIP("Test doesn't work because the names of serial ports aren't set found env.", SkipAll);
#endif
    } else {
        m_availablePortNames << m_senderPortName << m_receiverPortName;
    }
}

void tst_QSerialPort::defaultConstruct()
{
    QSerialPort serialPort;

    QCOMPARE(serialPort.error(), QSerialPort::NoError);
    QVERIFY(!serialPort.errorString().isEmpty());

    // properties
    const qint32 defaultBaudRate = static_cast<qint32>(QSerialPort::Baud9600);
    QCOMPARE(serialPort.baudRate(), defaultBaudRate);
    QCOMPARE(serialPort.baudRate(QSerialPort::Input), defaultBaudRate);
    QCOMPARE(serialPort.baudRate(QSerialPort::Output), defaultBaudRate);
    QCOMPARE(serialPort.dataBits(), QSerialPort::Data8);
    QCOMPARE(serialPort.parity(), QSerialPort::NoParity);
    QCOMPARE(serialPort.stopBits(), QSerialPort::OneStop);
    QCOMPARE(serialPort.flowControl(), QSerialPort::NoFlowControl);

    QCOMPARE(serialPort.pinoutSignals(), QSerialPort::NoSignal);
    QCOMPARE(serialPort.isRequestToSend(), false);
    QCOMPARE(serialPort.isDataTerminalReady(), false);

    // QIODevice
    QCOMPARE(serialPort.openMode(), QIODevice::NotOpen);
    QVERIFY(!serialPort.isOpen());
    QVERIFY(!serialPort.isReadable());
    QVERIFY(!serialPort.isWritable());
    QVERIFY(serialPort.isSequential());
    QCOMPARE(serialPort.canReadLine(), false);
    QCOMPARE(serialPort.pos(), qlonglong(0));
    QCOMPARE(serialPort.size(), qlonglong(0));
    QVERIFY(serialPort.atEnd());
    QCOMPARE(serialPort.bytesAvailable(), qlonglong(0));
    QCOMPARE(serialPort.bytesToWrite(), qlonglong(0));

    char c;
    QCOMPARE(serialPort.read(&c, 1), qlonglong(-1));
    QCOMPARE(serialPort.write(&c, 1), qlonglong(-1));
}

void tst_QSerialPort::constructByName()
{
    QSerialPort serialPort(m_senderPortName);
    QCOMPARE(serialPort.portName(), m_senderPortName);
    serialPort.setPortName(m_receiverPortName);
    QCOMPARE(serialPort.portName(), m_receiverPortName);
}

void tst_QSerialPort::constructByInfo()
{
    QSerialPortInfo senderPortInfo(m_senderPortName);
    QSerialPort serialPort(senderPortInfo);
    QCOMPARE(serialPort.portName(), m_senderPortName);
    QSerialPortInfo receiverPortInfo(m_receiverPortName);
    serialPort.setPort(receiverPortInfo);
    QCOMPARE(serialPort.portName(), m_receiverPortName);
}

void tst_QSerialPort::openExisting_data()
{
    QTest::addColumn<int>("openMode");
    QTest::addColumn<bool>("openResult");
    QTest::addColumn<QSerialPort::SerialPortError>("errorCode");

    QTest::newRow("NotOpen") << int(QIODevice::NotOpen) << false << QSerialPort::UnsupportedOperationError;
    QTest::newRow("ReadOnly") << int(QIODevice::ReadOnly) << true << QSerialPort::NoError;
    QTest::newRow("WriteOnly") << int(QIODevice::WriteOnly) << true << QSerialPort::NoError;
    QTest::newRow("ReadWrite") << int(QIODevice::ReadWrite) << true << QSerialPort::NoError;
    QTest::newRow("Append") << int(QIODevice::Append) << false << QSerialPort::UnsupportedOperationError;
    QTest::newRow("Truncate") << int(QIODevice::Truncate) << false << QSerialPort::UnsupportedOperationError;
    QTest::newRow("Text") << int(QIODevice::Text) << false << QSerialPort::UnsupportedOperationError;
    QTest::newRow("Unbuffered") << int(QIODevice::Unbuffered) << false << QSerialPort::UnsupportedOperationError;
}

void tst_QSerialPort::openExisting()
{
    QFETCH(int, openMode);
    QFETCH(bool, openResult);
    QFETCH(QSerialPort::SerialPortError, errorCode);

    foreach (const QString &serialPortName, m_availablePortNames) {
        QSerialPort serialPort(serialPortName);
        QSignalSpy errorSpy(&serialPort, SIGNAL(error(QSerialPort::SerialPortError)));
        QVERIFY(errorSpy.isValid());

        QCOMPARE(serialPort.portName(), serialPortName);
        QCOMPARE(serialPort.open(QIODevice::OpenMode(openMode)), openResult);
        QCOMPARE(serialPort.isOpen(), openResult);
        QCOMPARE(serialPort.error(), errorCode);

        QCOMPARE(errorSpy.count(), 1);
        QCOMPARE(qvariant_cast<QSerialPort::SerialPortError>(errorSpy.at(0).at(0)), errorCode);
    }
}

void tst_QSerialPort::openNotExisting_data()
{
    QTest::addColumn<QString>("serialPortName");
    QTest::addColumn<bool>("openResult");
    QTest::addColumn<QSerialPort::SerialPortError>("errorCode");

    QTest::newRow("Empty") << QString("") << false << QSerialPort::DeviceNotFoundError;
    QTest::newRow("Null") << QString() << false << QSerialPort::DeviceNotFoundError;
    QTest::newRow("NotExists") << QString("ABCDEF") << false << QSerialPort::DeviceNotFoundError;
}

void tst_QSerialPort::openNotExisting()
{
    QFETCH(QString, serialPortName);
    QFETCH(bool, openResult);
    //QFETCH(QSerialPort::SerialPortError, errorCode);

    QSerialPort serialPort(serialPortName);

    QSignalSpy errorSpy(&serialPort, SIGNAL(error(QSerialPort::SerialPortError)));
    QVERIFY(errorSpy.isValid());

    QCOMPARE(serialPort.portName(), serialPortName);
    QCOMPARE(serialPort.open(QIODevice::ReadOnly), openResult);
    QCOMPARE(serialPort.isOpen(), openResult);
    //QCOMPARE(serialPort.error(), errorCode);

    //QCOMPARE(errorSpy.count(), 1);
    //QCOMPARE(qvariant_cast<QSerialPort::SerialPortError>(errorSpy.at(0).at(0)), errorCode);
}

void tst_QSerialPort::handleBytesWrittenAndExitLoopSlot(qint64 bytesWritten)
{
    QCOMPARE(bytesWritten, qint64(alphabetArray.size() + newlineArray.size()));
    exitLoop();
}

void tst_QSerialPort::flush()
{
#ifdef Q_OS_WIN
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    QSKIP("flush() does not work on Windows");
#else
    QSKIP("flush() does not work on Windows", SkipAll);
#endif
#endif

    QSerialPort serialPort(m_senderPortName);
    connect(&serialPort, SIGNAL(bytesWritten(qint64)), this, SLOT(handleBytesWrittenAndExitLoopSlot(qint64)));
    QSignalSpy bytesWrittenSpy(&serialPort, SIGNAL(bytesWritten(qint64)));

    QVERIFY(serialPort.open(QIODevice::WriteOnly));
    serialPort.write(alphabetArray + newlineArray);
    QCOMPARE(serialPort.bytesToWrite(), qint64(alphabetArray.size() + newlineArray.size()));
    serialPort.flush();
    QCOMPARE(serialPort.bytesToWrite(), qint64(0));
    enterLoop(1);
    QVERIFY2(!timeout(), "Timed out when waiting for the bytesWritten(qint64) signal.");
    QCOMPARE(bytesWrittenSpy.count(), 1);
}

void tst_QSerialPort::handleBytesWrittenAndExitLoopSlot2(qint64 bytesWritten)
{
    static qint64 bytes = 0;
    bytes += bytesWritten;

    QVERIFY(bytesWritten == newlineArray.size() || bytesWritten == alphabetArray.size());

    if (bytes == (alphabetArray.size() + newlineArray.size()))
        exitLoop();
}

void tst_QSerialPort::doubleFlush()
{
#ifdef Q_OS_WIN
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    QSKIP("flush() does not work on Windows");
#else
    QSKIP("flush() does not work on Windows", SkipAll);
#endif
#endif

    QSerialPort serialPort(m_senderPortName);
    connect(&serialPort, SIGNAL(bytesWritten(qint64)), this, SLOT(handleBytesWrittenAndExitLoopSlot2(qint64)));
    QSignalSpy bytesWrittenSpy(&serialPort, SIGNAL(bytesWritten(qint64)));

    QVERIFY(serialPort.open(QIODevice::WriteOnly));
    serialPort.write(alphabetArray);
    QCOMPARE(serialPort.bytesToWrite(), qint64(alphabetArray.size()));
    serialPort.flush();
    QCOMPARE(serialPort.bytesToWrite(), qint64(0));
    serialPort.write(newlineArray);
    QCOMPARE(serialPort.bytesToWrite(), qint64(newlineArray.size()));
    serialPort.flush();
    QCOMPARE(serialPort.bytesToWrite(), qint64(0));

    enterLoop(1);
    QVERIFY2(!timeout(), "Timed out when waiting for the bytesWritten(qint64) signal.");
    QCOMPARE(bytesWrittenSpy.count(), 2);
}

void tst_QSerialPort::waitForBytesWritten()
{
    // the dummy device on other side also has to be open
    QSerialPort dummySerialPort(m_receiverPortName);
    QVERIFY(dummySerialPort.open(QIODevice::ReadOnly));

    QSerialPort serialPort(m_senderPortName);
    QVERIFY(serialPort.open(QIODevice::WriteOnly));
    serialPort.write(alphabetArray);
    const qint64 toWrite = serialPort.bytesToWrite();
    QVERIFY(serialPort.waitForBytesWritten(1000));
    QVERIFY(toWrite > serialPort.bytesToWrite());
}

void tst_QSerialPort::waitForReadyReadWithTimeout()
{
#ifdef Q_OS_WIN
    clearReceiver();
    // the dummy device on other side also has to be open
    QSerialPort dummySerialPort(m_senderPortName);
    QVERIFY(dummySerialPort.open(QIODevice::WriteOnly));
#endif

    QSerialPort receiverSerialPort(m_receiverPortName);
    QVERIFY(receiverSerialPort.open(QIODevice::ReadOnly));
    QVERIFY(!receiverSerialPort.waitForReadyRead(5));
    QCOMPARE(receiverSerialPort.bytesAvailable(), qint64(0));
    QCOMPARE(receiverSerialPort.error(), QSerialPort::TimeoutError);
}

void tst_QSerialPort::waitForReadyReadWithOneByte()
{
#ifdef Q_OS_WIN
    clearReceiver();
#endif

    const qint64 oneByte = 1;
    const int waitMsecs = 50;

    QSerialPort senderSerialPort(m_senderPortName);
    QVERIFY(senderSerialPort.open(QIODevice::WriteOnly));
    QSerialPort receiverSerialPort(m_receiverPortName);
    QSignalSpy readyReadSpy(&receiverSerialPort, SIGNAL(readyRead()));
    QVERIFY(readyReadSpy.isValid());
    QVERIFY(receiverSerialPort.open(QIODevice::ReadOnly));
    QCOMPARE(senderSerialPort.write(alphabetArray.constData(), oneByte), oneByte);
    QVERIFY(senderSerialPort.waitForBytesWritten(waitMsecs));
    QVERIFY(receiverSerialPort.waitForReadyRead(waitMsecs));
    QCOMPARE(receiverSerialPort.bytesAvailable(), oneByte);
    QCOMPARE(receiverSerialPort.error(), QSerialPort::NoError);
    QCOMPARE(readyReadSpy.count(), 1);
}

void tst_QSerialPort::waitForReadyReadWithAlphabet()
{
#ifdef Q_OS_WIN
    clearReceiver();
#endif

    const int waitMsecs = 50;

    QSerialPort senderSerialPort(m_senderPortName);
    QVERIFY(senderSerialPort.open(QIODevice::WriteOnly));
    QSerialPort receiverSerialPort(m_receiverPortName);
    QSignalSpy readyReadSpy(&receiverSerialPort, SIGNAL(readyRead()));
    QVERIFY(readyReadSpy.isValid());
    QVERIFY(receiverSerialPort.open(QIODevice::ReadOnly));
    QCOMPARE(senderSerialPort.write(alphabetArray), qint64(alphabetArray.size()));
    QVERIFY(senderSerialPort.waitForBytesWritten(waitMsecs));

    do {
        QVERIFY(receiverSerialPort.waitForReadyRead(waitMsecs));
    } while (receiverSerialPort.bytesAvailable() < qint64(alphabetArray.size()));

    QCOMPARE(receiverSerialPort.error(), QSerialPort::NoError);
    QVERIFY(readyReadSpy.count() > 0);
}

QTEST_MAIN(tst_QSerialPort)
#include "tst_qserialport.moc"
