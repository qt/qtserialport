// Copyright (C) 2014 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>
#include <QtTest/private/qpropertytesthelper_p.h>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

#include <QThread>

#ifdef Q_OS_UNIX
#include <private/qcore_unix_p.h>
#include <termios.h>
#elif defined(Q_OS_WIN)
#include <QtCore/qt_windows.h>
#endif

#ifdef Q_OS_QNX
#  ifdef open
#    undef open
#  endif
#endif

Q_DECLARE_METATYPE(QSerialPort::SerialPortError);
Q_DECLARE_METATYPE(QSerialPort::BaudRate);
Q_DECLARE_METATYPE(QSerialPort::DataBits);
Q_DECLARE_METATYPE(QSerialPort::Parity);
Q_DECLARE_METATYPE(QSerialPort::StopBits);
Q_DECLARE_METATYPE(QSerialPort::FlowControl);
Q_DECLARE_METATYPE(QIODevice::OpenMode);
Q_DECLARE_METATYPE(QIODevice::OpenModeFlag);
Q_DECLARE_METATYPE(Qt::ConnectionType);

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
        QTestEventLoop::instance().enterLoopMSecs(msecs);
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

    void baudRate_data();
    void baudRate();
    void dataBits_data();
    void dataBits();
    void parity_data();
    void parity();
    void stopBits_data();
    void stopBits();
    void flowControl_data();
    void flowControl();

    void rts();
    void dtr();
    void independenceRtsAndDtr();

    void flush();
    void doubleFlush();

    void waitForBytesWritten();

    void waitForReadyReadWithTimeout();
    void waitForReadyReadWithOneByte();
    void waitForReadyReadWithAlphabet();

    void twoStageSynchronousLoopback();

    void synchronousReadWrite();

    void asynchronousWriteByBytesWritten_data();
    void asynchronousWriteByBytesWritten();

    void asynchronousWriteByTimer_data();
    void asynchronousWriteByTimer();

    void asyncReadWithLimitedReadBufferSize();

    void readBufferOverflow();
    void readAfterInputClear();
    void synchronousReadWriteAfterAsynchronousReadWrite();

    void controlBreak();

    void clearAfterOpen();

    void readWriteWithDifferentBaudRate_data();
    void readWriteWithDifferentBaudRate();

    void readWriteWithMarkSpaceParity_data();
    void readWriteWithMarkSpaceParity();

    void bindingsAndProperties();

    void restoreSettingsOnClose();

    void readyReadNotRecursive();

    void writeBufferSizeLimit_data();
    void writeBufferSizeLimit();

protected slots:
    void handleBytesWrittenAndExitLoopSlot(qint64 bytesWritten);
    void handleBytesWrittenAndExitLoopSlot2(qint64 bytesWritten);

private:
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
    qRegisterMetaType<QSerialPort::SerialPortError>("QSerialPort::SerialPortError");
}

void tst_QSerialPort::initTestCase()
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
              ", like: ttyS0\n";
#elif defined(Q_OS_WIN32)
              ", like: COM1\n";
#else
              "\n";
#endif

        QSKIP(message);
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
    QCOMPARE(serialPort.settingsRestoredOnClose(), true);

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
    QSerialPortInfo receiverPortInfo(m_receiverPortName);

#if defined(Q_OS_UNIX)
    if (senderPortInfo.isNull() || receiverPortInfo.isNull()) {
        static const char message[] =
                "Test doesn't work because the specified serial ports aren't"
                " found in system and can't be constructed by QSerialPortInfo.\n";
        QSKIP(message);
    }
#endif

    QSerialPort serialPort(senderPortInfo);
    QCOMPARE(serialPort.portName(), m_senderPortName);
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

    for (const QString &serialPortName : std::as_const(m_availablePortNames)) {
        QSerialPort serialPort(serialPortName);
        QSignalSpy errorSpy(&serialPort, &QSerialPort::errorOccurred);
        QVERIFY(errorSpy.isValid());

        QCOMPARE(serialPort.portName(), serialPortName);
        QCOMPARE(serialPort.open(QIODevice::OpenMode(openMode)), openResult);
        QCOMPARE(serialPort.isOpen(), openResult);
        QCOMPARE(serialPort.error(), errorCode);

        QCOMPARE(errorSpy.size(), 1);
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

    QSignalSpy errorSpy(&serialPort, &QSerialPort::errorOccurred);
    QVERIFY(errorSpy.isValid());

    QCOMPARE(serialPort.portName(), serialPortName);
    QCOMPARE(serialPort.open(QIODevice::ReadOnly), openResult);
    QCOMPARE(serialPort.isOpen(), openResult);
    //QCOMPARE(serialPort.error(), errorCode);

    //QCOMPARE(errorSpy.count(), 1);
    //QCOMPARE(qvariant_cast<QSerialPort::SerialPortError>(errorSpy.at(0).at(0)), errorCode);
}

void tst_QSerialPort::baudRate_data()
{
    QTest::addColumn<qint32>("baudrate");
    QTest::newRow("Baud1200") << static_cast<qint32>(QSerialPort::Baud1200);
    QTest::newRow("Baud2400") << static_cast<qint32>(QSerialPort::Baud2400);
    QTest::newRow("Baud4800") << static_cast<qint32>(QSerialPort::Baud4800);
    QTest::newRow("Baud9600") << static_cast<qint32>(QSerialPort::Baud9600);
    QTest::newRow("Baud19200") << static_cast<qint32>(QSerialPort::Baud19200);
    QTest::newRow("Baud38400") << static_cast<qint32>(QSerialPort::Baud38400);
    QTest::newRow("Baud57600") << static_cast<qint32>(QSerialPort::Baud57600);
    QTest::newRow("Baud115200") << static_cast<qint32>(QSerialPort::Baud115200);

    QTest::newRow("31250") << 31250; // custom baudrate (MIDI)
}

void tst_QSerialPort::baudRate()
{
    QFETCH(qint32, baudrate);

    {
        // setup before opening
        QSerialPort serialPort(m_senderPortName);
        QVERIFY(serialPort.setBaudRate(baudrate));
        QCOMPARE(serialPort.baudRate(), baudrate);
        QVERIFY(serialPort.open(QIODevice::ReadWrite));
    }

    {
        // setup after opening
        QSerialPort serialPort(m_senderPortName);
        QVERIFY(serialPort.open(QIODevice::ReadWrite));
        QVERIFY(serialPort.setBaudRate(baudrate));
        QCOMPARE(serialPort.baudRate(), baudrate);
    }
}

void tst_QSerialPort::dataBits_data()
{
    QTest::addColumn<QSerialPort::DataBits>("databits");
    QTest::newRow("Data5") << QSerialPort::Data5;
    QTest::newRow("Data6") << QSerialPort::Data6;
    QTest::newRow("Data7") << QSerialPort::Data7;
    QTest::newRow("Data8") << QSerialPort::Data8;
}

void tst_QSerialPort::dataBits()
{
    QFETCH(QSerialPort::DataBits, databits);

    {
        // setup before opening
        QSerialPort serialPort(m_senderPortName);
        QVERIFY(serialPort.setDataBits(databits));
        QCOMPARE(serialPort.dataBits(), databits);
        QVERIFY(serialPort.open(QIODevice::ReadWrite));
    }

    {
        // setup after opening
        QSerialPort serialPort(m_senderPortName);
        QVERIFY(serialPort.open(QIODevice::ReadWrite));
        QVERIFY(serialPort.setDataBits(databits));
        QCOMPARE(serialPort.dataBits(), databits);
    }
}

void tst_QSerialPort::parity_data()
{
    QTest::addColumn<QSerialPort::Parity>("parity");
    QTest::newRow("NoParity") << QSerialPort::NoParity;
    QTest::newRow("EvenParity") << QSerialPort::EvenParity;
    QTest::newRow("OddParity") << QSerialPort::OddParity;
    QTest::newRow("SpaceParity") << QSerialPort::SpaceParity;
    QTest::newRow("MarkParity") << QSerialPort::MarkParity;
}

void tst_QSerialPort::parity()
{
    QFETCH(QSerialPort::Parity, parity);

    {
        // setup before opening
        QSerialPort serialPort(m_senderPortName);
        QVERIFY(serialPort.setParity(parity));
        QCOMPARE(serialPort.parity(), parity);
        QVERIFY(serialPort.open(QIODevice::ReadWrite));
    }

    {
        // setup after opening
        QSerialPort serialPort(m_senderPortName);
        QVERIFY(serialPort.open(QIODevice::ReadWrite));
        QVERIFY(serialPort.setParity(parity));
        QCOMPARE(serialPort.parity(), parity);
    }
}

void tst_QSerialPort::stopBits_data()
{
    QTest::addColumn<QSerialPort::StopBits>("stopbits");
    QTest::newRow("OneStop") << QSerialPort::OneStop;
#ifdef Q_OS_WIN
    QTest::newRow("OneAndHalfStop") << QSerialPort::OneAndHalfStop;
#endif
    QTest::newRow("TwoStop") << QSerialPort::TwoStop;
}

void tst_QSerialPort::stopBits()
{
    QFETCH(QSerialPort::StopBits, stopbits);

    {
        // setup before opening
        QSerialPort serialPort(m_senderPortName);
        QVERIFY(serialPort.setStopBits(stopbits));
        QCOMPARE(serialPort.stopBits(), stopbits);
        QVERIFY(serialPort.open(QIODevice::ReadWrite));
    }

    {
        // setup after opening
        QSerialPort serialPort(m_senderPortName);
        QVERIFY(serialPort.open(QIODevice::ReadWrite));
        QVERIFY(serialPort.setStopBits(stopbits));
        QCOMPARE(serialPort.stopBits(), stopbits);
    }
}

void tst_QSerialPort::flowControl_data()
{
    QTest::addColumn<QSerialPort::FlowControl>("flowcontrol");
    QTest::newRow("NoFlowControl") << QSerialPort::NoFlowControl;
    QTest::newRow("HardwareControl") << QSerialPort::HardwareControl;
    QTest::newRow("SoftwareControl") << QSerialPort::SoftwareControl;
}

void tst_QSerialPort::flowControl()
{
    QFETCH(QSerialPort::FlowControl, flowcontrol);

    {
        // setup before opening
        QSerialPort serialPort(m_senderPortName);
        QVERIFY(serialPort.setFlowControl(flowcontrol));
        QCOMPARE(serialPort.flowControl(), flowcontrol);
        QVERIFY(serialPort.open(QIODevice::ReadWrite));
    }

    {
        // setup after opening
        QSerialPort serialPort(m_senderPortName);
        QVERIFY(serialPort.open(QIODevice::ReadWrite));
        QVERIFY(serialPort.setFlowControl(flowcontrol));
        QCOMPARE(serialPort.flowControl(), flowcontrol);
    }
}

void tst_QSerialPort::rts()
{
    QSerialPort serialPort(m_senderPortName);

    QSignalSpy errorSpy(&serialPort, &QSerialPort::errorOccurred);
    QVERIFY(errorSpy.isValid());
    QSignalSpy rtsSpy(&serialPort, &QSerialPort::requestToSendChanged);
    QVERIFY(rtsSpy.isValid());

    QVERIFY(serialPort.open(QIODevice::ReadWrite));

    // no flow control
    QVERIFY(serialPort.setFlowControl(QSerialPort::NoFlowControl));
    const bool toggle1 = !serialPort.isRequestToSend();
    QVERIFY(serialPort.setRequestToSend(toggle1));
    QCOMPARE(serialPort.isRequestToSend(), toggle1);

    // software flow control
    QVERIFY(serialPort.setFlowControl(QSerialPort::SoftwareControl));
    const bool toggle2 = !serialPort.isRequestToSend();
    QVERIFY(serialPort.setRequestToSend(toggle2));
    QCOMPARE(serialPort.isRequestToSend(), toggle2);

    // hardware flow control
    QVERIFY(serialPort.setFlowControl(QSerialPort::HardwareControl));
    const bool toggle3 = !serialPort.isRequestToSend();
    QVERIFY(!serialPort.setRequestToSend(toggle3)); // not allowed
    QCOMPARE(serialPort.isRequestToSend(), !toggle3); // same as before
    QCOMPARE(serialPort.error(), QSerialPort::UnsupportedOperationError);

    QCOMPARE(errorSpy.size(), 2);
    QCOMPARE(qvariant_cast<QSerialPort::SerialPortError>(errorSpy.at(0).at(0)), QSerialPort::NoError);
    QCOMPARE(qvariant_cast<QSerialPort::SerialPortError>(errorSpy.at(1).at(0)), QSerialPort::UnsupportedOperationError);

    QCOMPARE(rtsSpy.size(), 2);
    QCOMPARE(qvariant_cast<bool>(rtsSpy.at(0).at(0)), toggle1);
    QCOMPARE(qvariant_cast<bool>(rtsSpy.at(1).at(0)), toggle2);
}

void tst_QSerialPort::dtr()
{
    QSerialPort serialPort(m_senderPortName);

    QSignalSpy errorSpy(&serialPort, &QSerialPort::errorOccurred);
    QVERIFY(errorSpy.isValid());
    QSignalSpy dtrSpy(&serialPort, &QSerialPort::dataTerminalReadyChanged);
    QVERIFY(dtrSpy.isValid());

    QVERIFY(serialPort.open(QIODevice::ReadWrite));

    // no flow control
    QVERIFY(serialPort.setFlowControl(QSerialPort::NoFlowControl));
    const bool toggle1 = !serialPort.isDataTerminalReady();
    QVERIFY(serialPort.setDataTerminalReady(toggle1));
    QCOMPARE(serialPort.isDataTerminalReady(), toggle1);

    // software flow control
    QVERIFY(serialPort.setFlowControl(QSerialPort::SoftwareControl));
    const bool toggle2 = !serialPort.isDataTerminalReady();
    QVERIFY(serialPort.setDataTerminalReady(toggle2));
    QCOMPARE(serialPort.isDataTerminalReady(), toggle2);

    // hardware flow control
    QVERIFY(serialPort.setFlowControl(QSerialPort::HardwareControl));
    const bool toggle3 = !serialPort.isDataTerminalReady();
    QVERIFY(serialPort.setDataTerminalReady(toggle3));
    QCOMPARE(serialPort.isDataTerminalReady(), toggle3);

    QCOMPARE(errorSpy.size(), 1);
    QCOMPARE(qvariant_cast<QSerialPort::SerialPortError>(errorSpy.at(0).at(0)), QSerialPort::NoError);

    QCOMPARE(dtrSpy.size(), 3);
    QCOMPARE(qvariant_cast<bool>(dtrSpy.at(0).at(0)), toggle1);
    QCOMPARE(qvariant_cast<bool>(dtrSpy.at(1).at(0)), toggle2);
    QCOMPARE(qvariant_cast<bool>(dtrSpy.at(2).at(0)), toggle3);
}

void tst_QSerialPort::independenceRtsAndDtr()
{
    QSerialPort serialPort(m_senderPortName);
    QVERIFY(serialPort.open(QIODevice::ReadWrite)); // No flow control by default!

    QVERIFY(serialPort.setDataTerminalReady(true));
    QVERIFY(serialPort.setRequestToSend(true));
    QVERIFY(serialPort.isDataTerminalReady());
    QVERIFY(serialPort.isRequestToSend());

    // check that DTR changing does not change RTS
    QVERIFY(serialPort.setDataTerminalReady(false));
    QVERIFY(!serialPort.isDataTerminalReady());
    QVERIFY(serialPort.isRequestToSend());
    QVERIFY(serialPort.setDataTerminalReady(true));
    QVERIFY(serialPort.isDataTerminalReady());
    QVERIFY(serialPort.isRequestToSend());

    // check that RTS changing does not change DTR
    QVERIFY(serialPort.setRequestToSend(false));
    QVERIFY(!serialPort.isRequestToSend());
    QVERIFY(serialPort.isDataTerminalReady());
    QVERIFY(serialPort.setRequestToSend(true));
    QVERIFY(serialPort.isRequestToSend());
    QVERIFY(serialPort.isDataTerminalReady());

    // check that baud rate changing does not change DTR or RTS
    QVERIFY(serialPort.setBaudRate(115200));
    QVERIFY(serialPort.isRequestToSend());
    QVERIFY(serialPort.isDataTerminalReady());

    // check that data bits changing does not change DTR or RTS
    QVERIFY(serialPort.setDataBits(QSerialPort::Data7));
    QVERIFY(serialPort.isRequestToSend());
    QVERIFY(serialPort.isDataTerminalReady());

    // check that parity changing does not change DTR or RTS
    QVERIFY(serialPort.setParity(QSerialPort::EvenParity));
    QVERIFY(serialPort.isRequestToSend());
    QVERIFY(serialPort.isDataTerminalReady());

    // check that stop bits changing does not change DTR or RTS
    QVERIFY(serialPort.setStopBits(QSerialPort::TwoStop));
    QVERIFY(serialPort.isRequestToSend());
    QVERIFY(serialPort.isDataTerminalReady());

    // check that software flow control changing does not change DTR or RTS
    QVERIFY(serialPort.setFlowControl(QSerialPort::SoftwareControl));
    QVERIFY(serialPort.isRequestToSend());
    QVERIFY(serialPort.isDataTerminalReady());
}

void tst_QSerialPort::handleBytesWrittenAndExitLoopSlot(qint64 bytesWritten)
{
    QCOMPARE(bytesWritten, qint64(alphabetArray.size() + newlineArray.size()));
    exitLoop();
}

void tst_QSerialPort::flush()
{
#ifdef Q_OS_WIN
    QSKIP("flush() does not work on Windows");
#endif

    // the dummy device on other side also has to be open
    QSerialPort dummySerialPort(m_receiverPortName);
    QVERIFY(dummySerialPort.open(QIODevice::ReadOnly));

    QSerialPort serialPort(m_senderPortName);
    connect(&serialPort, &QSerialPort::bytesWritten, this, &tst_QSerialPort::handleBytesWrittenAndExitLoopSlot);
    QSignalSpy bytesWrittenSpy(&serialPort, &QSerialPort::bytesWritten);

    QVERIFY(serialPort.open(QIODevice::WriteOnly));
    serialPort.write(alphabetArray + newlineArray);
    QCOMPARE(serialPort.bytesToWrite(), qint64(alphabetArray.size() + newlineArray.size()));
    serialPort.flush();
    QCOMPARE(serialPort.bytesToWrite(), qint64(0));
    enterLoop(1);
    QVERIFY2(!timeout(), "Timed out when waiting for the bytesWritten(qint64) signal.");
    QCOMPARE(bytesWrittenSpy.size(), 1);
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
    QSKIP("flush() does not work on Windows");
#endif

    // the dummy device on other side also has to be open
    QSerialPort dummySerialPort(m_receiverPortName);
    QVERIFY(dummySerialPort.open(QIODevice::ReadOnly));

    QSerialPort serialPort(m_senderPortName);
    connect(&serialPort, &QSerialPort::bytesWritten, this, &tst_QSerialPort::handleBytesWrittenAndExitLoopSlot2);
    QSignalSpy bytesWrittenSpy(&serialPort, &QSerialPort::bytesWritten);

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
    QCOMPARE(bytesWrittenSpy.size(), 2);
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
    // the dummy device on other side also has to be open
    QSerialPort dummySerialPort(m_senderPortName);
    QVERIFY(dummySerialPort.open(QIODevice::WriteOnly));

    QSerialPort receiverSerialPort(m_receiverPortName);
    QVERIFY(receiverSerialPort.open(QIODevice::ReadOnly));
    QVERIFY(!receiverSerialPort.waitForReadyRead(5));
    QCOMPARE(receiverSerialPort.bytesAvailable(), qint64(0));
    QCOMPARE(receiverSerialPort.error(), QSerialPort::TimeoutError);
}

void tst_QSerialPort::waitForReadyReadWithOneByte()
{
    const qint64 oneByte = 1;
    const int waitMsecs = 50;

    QSerialPort senderSerialPort(m_senderPortName);
    QVERIFY(senderSerialPort.open(QIODevice::WriteOnly));
    QSerialPort receiverSerialPort(m_receiverPortName);
    QSignalSpy readyReadSpy(&receiverSerialPort, &QSerialPort::readyRead);
    QVERIFY(readyReadSpy.isValid());
    QVERIFY(receiverSerialPort.open(QIODevice::ReadOnly));
    QCOMPARE(senderSerialPort.write(alphabetArray.constData(), oneByte), oneByte);
    QVERIFY(senderSerialPort.waitForBytesWritten(waitMsecs));
    QVERIFY(receiverSerialPort.waitForReadyRead(waitMsecs));
    QCOMPARE(receiverSerialPort.bytesAvailable(), oneByte);
    QCOMPARE(receiverSerialPort.error(), QSerialPort::NoError);
    QCOMPARE(readyReadSpy.size(), 1);
}

void tst_QSerialPort::waitForReadyReadWithAlphabet()
{
    const int waitMsecs = 50;

    QSerialPort senderSerialPort(m_senderPortName);
    QVERIFY(senderSerialPort.open(QIODevice::WriteOnly));
    QSerialPort receiverSerialPort(m_receiverPortName);
    QSignalSpy readyReadSpy(&receiverSerialPort, &QSerialPort::readyRead);
    QVERIFY(readyReadSpy.isValid());
    QVERIFY(receiverSerialPort.open(QIODevice::ReadOnly));
    QCOMPARE(senderSerialPort.write(alphabetArray), qint64(alphabetArray.size()));
    QVERIFY(senderSerialPort.waitForBytesWritten(waitMsecs));

    do {
        QVERIFY(receiverSerialPort.waitForReadyRead(waitMsecs));
    } while (receiverSerialPort.bytesAvailable() < qint64(alphabetArray.size()));

    QCOMPARE(receiverSerialPort.error(), QSerialPort::NoError);
    QVERIFY(readyReadSpy.size() > 0);
}

void tst_QSerialPort::twoStageSynchronousLoopback()
{
    QSerialPort senderPort(m_senderPortName);
    QVERIFY(senderPort.open(QSerialPort::ReadWrite));

    QSerialPort receiverPort(m_receiverPortName);
    QVERIFY(receiverPort.open(QSerialPort::ReadWrite));

    const int waitMsecs = 50;

    // first stage
    senderPort.write(newlineArray);
    senderPort.waitForBytesWritten(waitMsecs);
    QTest::qSleep(waitMsecs);
    receiverPort.waitForReadyRead(waitMsecs);
    QCOMPARE(receiverPort.bytesAvailable(), qint64(newlineArray.size()));

    receiverPort.write(receiverPort.readAll());
    receiverPort.waitForBytesWritten(waitMsecs);
    QTest::qSleep(waitMsecs);
    senderPort.waitForReadyRead(waitMsecs);
    QCOMPARE(senderPort.bytesAvailable(), qint64(newlineArray.size()));
    QCOMPARE(senderPort.readAll(), newlineArray);

    // second stage
    senderPort.write(newlineArray);
    senderPort.waitForBytesWritten(waitMsecs);
    QTest::qSleep(waitMsecs);
    receiverPort.waitForReadyRead(waitMsecs);
    QCOMPARE(receiverPort.bytesAvailable(), qint64(newlineArray.size()));
    receiverPort.write(receiverPort.readAll());
    receiverPort.waitForBytesWritten(waitMsecs);
    QTest::qSleep(waitMsecs);
    senderPort.waitForReadyRead(waitMsecs);
    QCOMPARE(senderPort.bytesAvailable(), qint64(newlineArray.size()));
    QCOMPARE(senderPort.readAll(), newlineArray);
}

void tst_QSerialPort::synchronousReadWrite()
{
    QSerialPort senderPort(m_senderPortName);
    QVERIFY(senderPort.open(QSerialPort::WriteOnly));

    QSerialPort receiverPort(m_receiverPortName);
    QVERIFY(receiverPort.open(QSerialPort::ReadOnly));

    QByteArray writeData;
    for (int i = 0; i < 1024; ++i)
        writeData.append(static_cast<char>(i));

    senderPort.write(writeData);
    senderPort.waitForBytesWritten(-1);

    QByteArray readData;
    while ((readData.size() < writeData.size()) && receiverPort.waitForReadyRead(100))
        readData.append(receiverPort.readAll());

    QCOMPARE(readData, writeData);
}

class AsyncReader : public QObject
{
    Q_OBJECT
public:
    explicit AsyncReader(QSerialPort &port, Qt::ConnectionType connectionType, int expectedBytesCount)
        : serialPort(port), expectedBytesCount(expectedBytesCount)
    {
        connect(&serialPort, &QSerialPort::readyRead, this, &AsyncReader::receive, connectionType);
    }

private slots:
    void receive()
    {
        if (serialPort.bytesAvailable() < expectedBytesCount)
            return;
        tst_QSerialPort::exitLoop();
    }

private:
    QSerialPort &serialPort;
    const int expectedBytesCount;
};

class AsyncWriterByBytesWritten : public QObject
{
    Q_OBJECT
public:
    explicit AsyncWriterByBytesWritten(
            QSerialPort &port, Qt::ConnectionType connectionType, const QByteArray &dataToWrite)
        : serialPort(port), writeChunkSize(0)
    {
        writeBuffer.setData(dataToWrite);
        writeBuffer.open(QIODevice::ReadOnly);
        connect(&serialPort, &QSerialPort::bytesWritten, this, &AsyncWriterByBytesWritten::send, connectionType);
        send();
    }

private slots:
    void send()
    {
        if (writeBuffer.bytesAvailable() > 0)
            serialPort.write(writeBuffer.read(++writeChunkSize));
    }

private:
    QSerialPort &serialPort;
    QBuffer writeBuffer;
    int writeChunkSize;
};

void tst_QSerialPort::asynchronousWriteByBytesWritten_data()
{
    QTest::addColumn<Qt::ConnectionType>("readConnectionType");
    QTest::addColumn<Qt::ConnectionType>("writeConnectionType");

    QTest::newRow("BothQueued") << Qt::QueuedConnection << Qt::QueuedConnection;
    QTest::newRow("BothDirect") << Qt::DirectConnection << Qt::DirectConnection;
    QTest::newRow("ReadDirectWriteQueued") << Qt::DirectConnection << Qt::QueuedConnection;
    QTest::newRow("ReadQueuedWriteDirect") << Qt::QueuedConnection << Qt::DirectConnection;
}

void tst_QSerialPort::asynchronousWriteByBytesWritten()
{
    QFETCH(Qt::ConnectionType, readConnectionType);
    QFETCH(Qt::ConnectionType, writeConnectionType);

    QSerialPort receiverPort(m_receiverPortName);
    QVERIFY(receiverPort.open(QSerialPort::ReadOnly));
    AsyncReader reader(receiverPort, readConnectionType, alphabetArray.size());

    QSerialPort senderPort(m_senderPortName);
    QVERIFY(senderPort.open(QSerialPort::WriteOnly));
    AsyncWriterByBytesWritten writer(senderPort, writeConnectionType, alphabetArray);

    enterLoop(1);
    QVERIFY2(!timeout(), "Timed out when waiting for the read or write.");
    QCOMPARE(receiverPort.bytesAvailable(), qint64(alphabetArray.size()));
    QCOMPARE(receiverPort.readAll(), alphabetArray);
}

class AsyncWriterByTimer : public QObject
{
    Q_OBJECT
public:
    explicit AsyncWriterByTimer(QSerialPort &port, Qt::ConnectionType connectionType,
                                const QByteArray &dataToWrite, int interval = 0)
        : serialPort(port), writeChunkSize(0), timerMsec(interval)
    {
        writeBuffer.setData(dataToWrite);
        writeBuffer.open(QIODevice::ReadOnly);
        connect(&timer, &QTimer::timeout, this, &AsyncWriterByTimer::send, connectionType);
        timer.start(timerMsec);
    }

private slots:
    void send()
    {
        if (writeBuffer.bytesAvailable() > 0)
            serialPort.write(writeBuffer.read(++writeChunkSize));
        else
            timer.stop();
    }

private:
    QSerialPort &serialPort;
    QBuffer writeBuffer;
    int writeChunkSize;
    QTimer timer;
    int timerMsec;
};

void tst_QSerialPort::asynchronousWriteByTimer_data()
{
    QTest::addColumn<Qt::ConnectionType>("readConnectionType");
    QTest::addColumn<Qt::ConnectionType>("writeConnectionType");

    QTest::newRow("BothQueued") << Qt::QueuedConnection << Qt::QueuedConnection;
    QTest::newRow("BothDirect") << Qt::DirectConnection << Qt::DirectConnection;
    QTest::newRow("ReadDirectWriteQueued") << Qt::DirectConnection << Qt::QueuedConnection;
    QTest::newRow("ReadQueuedWriteDirect") << Qt::QueuedConnection << Qt::DirectConnection;
}

void tst_QSerialPort::asynchronousWriteByTimer()
{
    QFETCH(Qt::ConnectionType, readConnectionType);
    QFETCH(Qt::ConnectionType, writeConnectionType);

    QSerialPort receiverPort(m_receiverPortName);
    QVERIFY(receiverPort.open(QSerialPort::ReadOnly));
    AsyncReader reader(receiverPort, readConnectionType, alphabetArray.size());

    QSerialPort senderPort(m_senderPortName);
    QVERIFY(senderPort.open(QSerialPort::WriteOnly));
    AsyncWriterByTimer writer(senderPort, writeConnectionType, alphabetArray);

    enterLoop(1);
    QVERIFY2(!timeout(), "Timed out when waiting for the read or write.");
    QCOMPARE(receiverPort.bytesAvailable(), qint64(alphabetArray.size()));
    QCOMPARE(receiverPort.readAll(), alphabetArray);
}

class AsyncReader2 : public QObject
{
    Q_OBJECT
public:
    explicit AsyncReader2(QSerialPort &port, const QByteArray &expectedData)
        : serialPort(port), expectedData(expectedData)
    {
        connect(&serialPort, &QSerialPort::readyRead, this, &AsyncReader2::receive);
    }

private slots:
    void receive()
    {
        receivedData.append(serialPort.readAll());
        if (receivedData == expectedData)
            tst_QSerialPort::exitLoop();
    }

private:
    QSerialPort &serialPort;
    const QByteArray expectedData;
    QByteArray receivedData;
};

void tst_QSerialPort::asyncReadWithLimitedReadBufferSize()
{
    QSerialPort senderPort(m_senderPortName);
    QVERIFY(senderPort.open(QSerialPort::WriteOnly));

    QSerialPort receiverPort(m_receiverPortName);
    QVERIFY(receiverPort.open(QSerialPort::ReadOnly));

    receiverPort.setReadBufferSize(1);
    QCOMPARE(receiverPort.readBufferSize(), qint64(1));

    AsyncReader2 reader(receiverPort, alphabetArray);

    QCOMPARE(senderPort.write(alphabetArray), qint64(alphabetArray.size()));

    enterLoop(1);
    QVERIFY2(!timeout(), "Timed out when waiting for the read or write.");
}

void tst_QSerialPort::readBufferOverflow()
{
    QSerialPort senderPort(m_senderPortName);
    QVERIFY(senderPort.open(QSerialPort::WriteOnly));

    QSerialPort receiverPort(m_receiverPortName);
    QVERIFY(receiverPort.open(QSerialPort::ReadOnly));

    const int readBufferSize = alphabetArray.size() / 2;
    receiverPort.setReadBufferSize(readBufferSize);
    QCOMPARE(receiverPort.readBufferSize(), qint64(readBufferSize));

    QCOMPARE(senderPort.write(alphabetArray), qint64(alphabetArray.size()));
    QVERIFY2(senderPort.waitForBytesWritten(100), "Waiting for bytes written failed");

    QByteArray readData;
    while (receiverPort.waitForReadyRead(100)) {
        QVERIFY(receiverPort.bytesAvailable() > 0);
        readData += receiverPort.readAll();
    }

    QCOMPARE(readData, alphabetArray);

    // No more bytes available
    QVERIFY(receiverPort.bytesAvailable() == 0);
}

void tst_QSerialPort::readAfterInputClear()
{
    QSerialPort senderPort(m_senderPortName);
    QVERIFY(senderPort.open(QSerialPort::WriteOnly));

    QSerialPort receiverPort(m_receiverPortName);
    QVERIFY(receiverPort.open(QSerialPort::ReadOnly));

    const int readBufferSize = alphabetArray.size() / 2;
    receiverPort.setReadBufferSize(readBufferSize);
    QCOMPARE(receiverPort.readBufferSize(), qint64(readBufferSize));

    const int waitMsecs = 100;

    // First write more than read buffer size
    QCOMPARE(senderPort.write(alphabetArray), qint64(alphabetArray.size()));
    QVERIFY2(senderPort.waitForBytesWritten(waitMsecs), "Waiting for bytes written failed");

    // Wait for first part of data into read buffer
    while (receiverPort.waitForReadyRead(waitMsecs));
    QCOMPARE(receiverPort.bytesAvailable(), qint64(readBufferSize));
    // Wait for second part of data into driver's FIFO
    QTest::qSleep(waitMsecs);

    QVERIFY(receiverPort.clear(QSerialPort::Input));
    QCOMPARE(receiverPort.bytesAvailable(), qint64(0));

    // Second write less than read buffer size
    QCOMPARE(senderPort.write(newlineArray), qint64(newlineArray.size()));
    QVERIFY2(senderPort.waitForBytesWritten(waitMsecs), "Waiting for bytes written failed");

    while (receiverPort.waitForReadyRead(waitMsecs));
    QCOMPARE(receiverPort.bytesAvailable(), qint64(newlineArray.size()));
    QCOMPARE(receiverPort.readAll(), newlineArray);

    // No more bytes available
    QVERIFY(receiverPort.bytesAvailable() == 0);
}

class SenderTransactor : public QObject
{
    Q_OBJECT
public:
    explicit SenderTransactor(const QString &name)
        : serialPort(name)
    {
    }

public slots:
    void open()
    {
        if (serialPort.open(QSerialPort::ReadWrite)) {
            createAsynchronousConnection();
            serialPort.write("A", 1);
        }
    }

private slots:
    void synchronousTransaction()
    {
        serialPort.write("B", 1);
        if (serialPort.waitForBytesWritten(100)) {
            if (serialPort.waitForReadyRead(100))
                tst_QSerialPort::exitLoop();
        }
    }

    void transaction()
    {
        deleteAsyncronousConnection();
        synchronousTransaction();
    }

private:
    void createAsynchronousConnection()
    {
        connect(&serialPort, &QSerialPort::readyRead, this, &SenderTransactor::transaction);
    }

    void deleteAsyncronousConnection()
    {
        serialPort.disconnect();
    }

    QSerialPort serialPort;
};

class ReceiverTransactor : public QObject
{
    Q_OBJECT
public:
    explicit ReceiverTransactor(const QString &name)
        : serialPort(new QSerialPort(name, this))
    {
        connect(serialPort, &QSerialPort::readyRead, this, &ReceiverTransactor::transaction);
    }

public slots:
    void open()
    {
        if (serialPort->open(QSerialPort::ReadWrite))
            emit ready();
    }

signals:
    void ready();

private slots:
    void transaction()
    {
        serialPort->write("Z", 1);
    }

private:
    QSerialPort *serialPort;
};

void tst_QSerialPort::synchronousReadWriteAfterAsynchronousReadWrite()
{
    SenderTransactor sender(m_senderPortName);
    auto receiver = new ReceiverTransactor(m_receiverPortName);

    QThread thread;
    receiver->moveToThread(&thread);
    thread.start();

    QObject::connect(&thread, &QThread::finished, receiver, &ReceiverTransactor::deleteLater);
    QObject::connect(receiver, &ReceiverTransactor::ready, &sender, &SenderTransactor::open);

    QMetaObject::invokeMethod(receiver, "open", Qt::QueuedConnection);

    tst_QSerialPort::enterLoopMsecs(2000);

    thread.quit();
    thread.wait();

    QVERIFY2(!timeout(), "Timed out when testing of transactions.");
}

class BreakReader : public QObject
{
    Q_OBJECT
public:
    explicit BreakReader(QSerialPort &port)
        : serialPort(port)
    {
        connect(&serialPort, &QSerialPort::readyRead, this, &BreakReader::receive);
    }

private slots:
    void receive()
    {
        tst_QSerialPort::exitLoop();
    }

private:
    QSerialPort &serialPort;
};

void tst_QSerialPort::controlBreak()
{
    QSerialPort senderPort(m_senderPortName);
    QVERIFY(senderPort.open(QSerialPort::WriteOnly));
    QCOMPARE(senderPort.isBreakEnabled(), false);

    QSignalSpy breakSpy(&senderPort, &QSerialPort::breakEnabledChanged);
    QVERIFY(breakSpy.isValid());

    QSerialPort receiverPort(m_receiverPortName);
    QVERIFY(receiverPort.open(QSerialPort::ReadOnly));

    BreakReader reader(receiverPort);

    QVERIFY(senderPort.setBreakEnabled(true));
    QCOMPARE(senderPort.isBreakEnabled(), true);

    enterLoop(1);
    QVERIFY2(!timeout(), "Timed out when waiting for the read of break state.");
    QVERIFY(receiverPort.bytesAvailable() > 0);

    const QByteArray actual = receiverPort.readAll();
    const QByteArray expected(actual.size(), '\0');
    QCOMPARE(actual, expected);

    QVERIFY(senderPort.setBreakEnabled(false));
    QCOMPARE(senderPort.isBreakEnabled(), false);

    QCOMPARE(breakSpy.size(), 2);
    QCOMPARE(qvariant_cast<bool>(breakSpy.at(0).at(0)), true);
    QCOMPARE(qvariant_cast<bool>(breakSpy.at(1).at(0)), false);
}

void tst_QSerialPort::clearAfterOpen()
{
    QSerialPort senderPort(m_senderPortName);
    QVERIFY(senderPort.open(QSerialPort::ReadWrite));
    QCOMPARE(senderPort.error(), QSerialPort::NoError);
    QVERIFY(senderPort.clear());
    QCOMPARE(senderPort.error(), QSerialPort::NoError);
}

void tst_QSerialPort::readWriteWithDifferentBaudRate_data()
{
    QTest::addColumn<int>("senderBaudRate");
    QTest::addColumn<int>("receiverBaudRate");
    QTest::addColumn<bool>("expectedResult");

    QTest::newRow("9600, 9600") << 9600 << 9600 << true;
    QTest::newRow("115200, 115200") << 115200 << 115200 << true;
    QTest::newRow("9600, 115200") << 9600 << 115200 << false;

    QTest::newRow("31250, 31250") << 31250 << 31250 << true; // custom baudrate (MIDI)
    QTest::newRow("31250, 115200") << 31250 << 115200 << false;

#ifdef Q_OS_LINUX
    QTest::newRow("14400, 14400") << 14400 << 14400 << true; // custom baudrate for Linux
    QTest::newRow("14400, 115200") << 14400 << 115200 << false;
#endif
}

void tst_QSerialPort::readWriteWithDifferentBaudRate()
{
    QFETCH(int, senderBaudRate);
    QFETCH(int, receiverBaudRate);
    QFETCH(bool, expectedResult);

    {
        // setup before opening
        QSerialPort senderSerialPort(m_senderPortName);
        QVERIFY(senderSerialPort.setBaudRate(senderBaudRate));
        QCOMPARE(senderSerialPort.baudRate(), senderBaudRate);
        QVERIFY(senderSerialPort.open(QSerialPort::ReadWrite));
        QSerialPort receiverSerialPort(m_receiverPortName);
        QVERIFY(receiverSerialPort.setBaudRate(receiverBaudRate));
        QCOMPARE(receiverSerialPort.baudRate(), receiverBaudRate);
        QVERIFY(receiverSerialPort.open(QSerialPort::ReadWrite));

        QCOMPARE(senderSerialPort.write(alphabetArray), qint64(alphabetArray.size()));
        QVERIFY(senderSerialPort.waitForBytesWritten(500));

        do {
            QVERIFY(receiverSerialPort.waitForReadyRead(500));
        } while (receiverSerialPort.bytesAvailable() < alphabetArray.size());

        if (expectedResult)
            QVERIFY(receiverSerialPort.readAll() == alphabetArray);
        else
            QVERIFY(receiverSerialPort.readAll() != alphabetArray);
    }

    {
        // setup after opening
        QSerialPort senderSerialPort(m_senderPortName);
        QVERIFY(senderSerialPort.open(QSerialPort::ReadWrite));
        QVERIFY(senderSerialPort.setBaudRate(senderBaudRate));
        QCOMPARE(senderSerialPort.baudRate(), senderBaudRate);
        QSerialPort receiverSerialPort(m_receiverPortName);
        QVERIFY(receiverSerialPort.open(QSerialPort::ReadWrite));
        QVERIFY(receiverSerialPort.setBaudRate(receiverBaudRate));
        QCOMPARE(receiverSerialPort.baudRate(), receiverBaudRate);

        QCOMPARE(senderSerialPort.write(alphabetArray), qint64(alphabetArray.size()));
        QVERIFY(senderSerialPort.waitForBytesWritten(500));

        do {
            QVERIFY(receiverSerialPort.waitForReadyRead(500));
        } while (receiverSerialPort.bytesAvailable() < alphabetArray.size());

        if (expectedResult)
            QVERIFY(receiverSerialPort.readAll() == alphabetArray);
        else
            QVERIFY(receiverSerialPort.readAll() != alphabetArray);
    }
}

void tst_QSerialPort::readWriteWithMarkSpaceParity_data()
{
    QTest::addColumn<QSerialPort::Parity>("parity");

    QTest::newRow("parity_space") << QSerialPort::SpaceParity;
    QTest::newRow("parity_mark") << QSerialPort::MarkParity;
}

void tst_QSerialPort::readWriteWithMarkSpaceParity()
{
    QFETCH(const QSerialPort::Parity, parity);

    auto setupPort = [&parity](QSerialPort &port) {
        port.setBaudRate(QSerialPort::Baud9600);
        port.setParity(parity);
        port.setDataBits(QSerialPort::Data8);
        port.setStopBits(QSerialPort::OneStop);
    };

    QSerialPort sender(m_senderPortName);
    QSignalSpy senderSpy(&sender, &QSerialPort::bytesWritten);
    setupPort(sender);
    QVERIFY(sender.open(QIODevice::ReadWrite));

    QSerialPort receiver(m_receiverPortName);
    QSignalSpy receiverSpy(&receiver, &QSerialPort::readyRead);
    setupPort(receiver);
    QVERIFY(receiver.open(QIODevice::ReadWrite));

    const QByteArray data("some data");
    const qint64 written = sender.write(data);
    QCOMPARE(written, qint64(data.size()));
    QTRY_COMPARE(senderSpy.size(), 1);
    QCOMPARE(senderSpy.at(0).at(0).value<qint64>(), written);

    QTRY_COMPARE_GE(receiver.bytesAvailable(), written);
    QCOMPARE_GE(receiverSpy.size(), 1); // we should get *at least one* signal

#if !defined(Q_OS_UNIX) || defined(CMSPAR)
    // On UNIX with no CMSPAR the result is flaky, so we cannot even use
    // QEXPECT_FAIL(). See QTBUG-131679.
    const QByteArray receivedData = receiver.readAll();
    QCOMPARE(receivedData, data);
#endif
}

void tst_QSerialPort::bindingsAndProperties()
{
    QSerialPort sp;

    // -- data bits

    QTestPrivate::testReadWritePropertyBasics(sp, QSerialPort::DataBits::Data6,
                                              QSerialPort::DataBits::Data5, "dataBits");
    if (QTest::currentTestFailed()) {
        qDebug("Failed property test for QSetialPort::dataBits");
        return;
    }

    // -- parity

    QTestPrivate::testReadWritePropertyBasics(sp, QSerialPort::Parity::SpaceParity,
                                              QSerialPort::Parity::EvenParity, "parity");
    if (QTest::currentTestFailed()) {
        qDebug("Failed property test for QSetialPort::parity");
        return;
    }

    // -- stop bits

    static_assert(std::is_same_v<decltype(sp.bindableStopBits()),
                                 QBindable<QSerialPort::StopBits>>);

    QTestPrivate::testReadWritePropertyBasics(sp, QSerialPort::StopBits::OneAndHalfStop,
                                              QSerialPort::StopBits::TwoStop, "stopBits");
    if (QTest::currentTestFailed()) {
        qDebug("Failed property test for QSetialPort::stopBits");
        return;
    }

    // -- flow control

    QTestPrivate::testReadWritePropertyBasics(sp, QSerialPort::FlowControl::HardwareControl,
                                              QSerialPort::FlowControl::SoftwareControl,
                                              "flowControl");
    if (QTest::currentTestFailed()) {
        qDebug("Failed property test for QSetialPort::flowControl");
        return;
    }

    // -- error

    QTestPrivate::testReadOnlyPropertyBasics(
            sp, QSerialPort::SerialPortError::NoError,
            QSerialPort::SerialPortError::UnsupportedOperationError, "error",
            [&sp]() { sp.open(QIODevice::Truncate); });
    if (QTest::currentTestFailed()) {
        qDebug("Failed property test for QSetialPort::error");
        return;
    }

    // -- break enabled

    sp.setPortName(m_receiverPortName);
    const bool portOpened = sp.open(QIODevice::ReadOnly);

    if (portOpened) {
        // To test the binding loop we need to create another instance of
        // QSerialPort, open it, and set breakEnabled == true as a default
        // value, so use a custom lambda for that.
        QTestPrivate::testReadWritePropertyBasics<QSerialPort, bool>(
                sp, true, false, "breakEnabled",
                [name = this->m_senderPortName]() {
                    auto ptr = std::make_unique<QSerialPort>();
                    ptr->setPortName(name);
                    if (ptr->open(QIODevice::ReadOnly)) {
                        ptr->setBreakEnabled(true);
                        return ptr;
                    }
                    // skip binding loop test if we failed to open the port
                    return std::unique_ptr<QSerialPort>{};
                });
        if (QTest::currentTestFailed()) {
            qDebug("Failed property test for QSetialPort::breakEnabled");
            return;
        }
    } else {
        // setting isBreakEnabled() will return false and raise an error
        sp.setBreakEnabled(true);
        QCOMPARE(sp.error(), QSerialPort::SerialPortError::NotOpenError);
    }
}

class PlatformSettings
{
public:
    explicit PlatformSettings(const QString &portName) : portInfo(portName) {}

    void getPortSettings();
    void restorePortSettings();
private:
    friend bool operator==(const PlatformSettings &lhs, const PlatformSettings &rhs);
    friend bool operator!=(const PlatformSettings &lhs, const PlatformSettings &rhs)
    { return !operator==(lhs, rhs); }


    QSerialPortInfo portInfo;
#ifdef Q_OS_UNIX
    termios tio;
#elif defined(Q_OS_WIN)
    DCB dcb;
    COMMTIMEOUTS commTimeouts;
#endif
};

void PlatformSettings::getPortSettings()
{
#ifdef Q_OS_UNIX
    const int flags = O_NOCTTY | O_NONBLOCK | O_RDONLY;
    int descriptor = qt_safe_open(portInfo.systemLocation().toLocal8Bit().constData(), flags);
    ::memset(&tio, 0, sizeof(termios));
    ::tcgetattr(descriptor, &tio);
    qt_safe_close(descriptor);
#elif defined(Q_OS_WIN)
    auto handle = ::CreateFile(reinterpret_cast<const wchar_t*>(portInfo.systemLocation().utf16()),
                               GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED,
                               nullptr);
    ::ZeroMemory(&dcb, sizeof(DCB));
    dcb.DCBlength = sizeof(DCB);
    ::GetCommState(handle, &dcb);
    ::GetCommTimeouts(handle, &commTimeouts);
    CloseHandle(handle);
#endif
}

void PlatformSettings::restorePortSettings()
{
#ifdef Q_OS_UNIX
    const int flags = O_NOCTTY | O_NONBLOCK | O_RDONLY;
    int descriptor = qt_safe_open(portInfo.systemLocation().toLocal8Bit().constData(), flags);
    ::tcsetattr(descriptor, TCSANOW, &tio);
    qt_safe_close(descriptor);
#elif defined(Q_OS_WIN)
    auto handle = ::CreateFile(reinterpret_cast<const wchar_t*>(portInfo.systemLocation().utf16()),
                               GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED,
                               nullptr);
    ::SetCommState(handle, &dcb);
    ::SetCommTimeouts(handle, &commTimeouts);
    CloseHandle(handle);
#endif
}

bool operator==(const PlatformSettings &lhs, const PlatformSettings &rhs)
{
#ifdef Q_OS_UNIX
    // compare only the minimal subset
    const bool tioEqual =
            lhs.tio.c_iflag == rhs.tio.c_iflag
            && lhs.tio.c_oflag == rhs.tio.c_oflag
            && lhs.tio.c_cflag == rhs.tio.c_cflag
            && lhs.tio.c_lflag == rhs.tio.c_lflag;

    if (tioEqual) {
        const auto lhsISpeed = ::cfgetispeed(&lhs.tio);
        const auto lhsOSpeed = ::cfgetospeed(&lhs.tio);

        const auto rhsISpeed = ::cfgetispeed(&rhs.tio);
        const auto rhsOSpeed = ::cfgetospeed(&rhs.tio);

        return lhsISpeed == rhsISpeed && lhsOSpeed == rhsOSpeed;
    }
    return tioEqual;
#elif defined(Q_OS_WIN)
    const bool timeoutsEqual =
        lhs.commTimeouts.ReadIntervalTimeout == rhs.commTimeouts.ReadIntervalTimeout
        && lhs.commTimeouts.ReadTotalTimeoutMultiplier == rhs.commTimeouts.ReadTotalTimeoutMultiplier
        && lhs.commTimeouts.ReadTotalTimeoutConstant == rhs.commTimeouts.ReadTotalTimeoutConstant
        && lhs.commTimeouts.WriteTotalTimeoutMultiplier == rhs.commTimeouts.WriteTotalTimeoutMultiplier
        && lhs.commTimeouts.WriteTotalTimeoutConstant == rhs.commTimeouts.WriteTotalTimeoutConstant;

    const bool dcbEqual =
        lhs.dcb.DCBlength == rhs.dcb.DCBlength
        && lhs.dcb.BaudRate == rhs.dcb.BaudRate
        && lhs.dcb.fBinary == rhs.dcb.fBinary
        && lhs.dcb.fParity == rhs.dcb.fParity
        && lhs.dcb.fOutxCtsFlow == rhs.dcb.fOutxCtsFlow
        && lhs.dcb.fOutxDsrFlow == rhs.dcb.fOutxDsrFlow
        && lhs.dcb.fDtrControl == rhs.dcb.fDtrControl
        && lhs.dcb.fDsrSensitivity == rhs.dcb.fDsrSensitivity
        && lhs.dcb.fTXContinueOnXoff == rhs.dcb.fTXContinueOnXoff
        && lhs.dcb.fOutX == rhs.dcb.fOutX
        && lhs.dcb.fInX == rhs.dcb.fInX
        && lhs.dcb.fErrorChar == rhs.dcb.fErrorChar
        && lhs.dcb.fNull == rhs.dcb.fNull
        && lhs.dcb.fRtsControl == rhs.dcb.fRtsControl
        && lhs.dcb.fAbortOnError == rhs.dcb.fAbortOnError
        && lhs.dcb.wReserved == rhs.dcb.wReserved
        && lhs.dcb.XonLim == rhs.dcb.XonLim
        && lhs.dcb.XoffLim == rhs.dcb.XoffLim
        && lhs.dcb.ByteSize == rhs.dcb.ByteSize
        && lhs.dcb.Parity == rhs.dcb.Parity
        && lhs.dcb.StopBits == rhs.dcb.StopBits
        && lhs.dcb.XonChar == rhs.dcb.XonChar
        && lhs.dcb.XoffChar == rhs.dcb.XoffChar
        && lhs.dcb.ErrorChar == rhs.dcb.ErrorChar
        && lhs.dcb.EofChar == rhs.dcb.EofChar
        && lhs.dcb.EvtChar == rhs.dcb.EvtChar;

    return timeoutsEqual && dcbEqual;
#endif
    // general case
    return false;
}

void tst_QSerialPort::restoreSettingsOnClose()
{
#ifndef Q_OS_APPLE
    PlatformSettings initialSettings(m_senderPortName);
    initialSettings.getPortSettings();

    QSerialPort port(m_senderPortName);
    QVERIFY(port.settingsRestoredOnClose());
    QSignalSpy restoreOnCloseSpy(&port, &QSerialPort::settingsRestoredOnCloseChanged);

    port.setSettingsRestoredOnClose(false);
    QVERIFY(!port.settingsRestoredOnClose());
    QCOMPARE(restoreOnCloseSpy.size(), 1);
    QCOMPARE(qvariant_cast<bool>(restoreOnCloseSpy.at(0).at(0)), false);

    port.setBaudRate(QSerialPort::Baud115200);
    port.setDataBits(QSerialPort::Data7);
    port.setStopBits(QSerialPort::OneStop);
    port.setParity(QSerialPort::OddParity);
    port.setFlowControl(QSerialPort::HardwareControl);

    // Open the port - new settings should be applied
    QVERIFY(port.open(QIODevice::ReadOnly));
    // Close the port - the same new settings should be kept
    port.close();

    PlatformSettings settingsAfterClose(m_senderPortName);
    settingsAfterClose.getPortSettings();

    QCOMPARE_NE(settingsAfterClose, initialSettings);

    port.setSettingsRestoredOnClose(true);
    QVERIFY(port.settingsRestoredOnClose());
    QCOMPARE(restoreOnCloseSpy.size(), 2);
    QCOMPARE(qvariant_cast<bool>(restoreOnCloseSpy.at(1).at(0)), true);

    port.setBaudRate(QSerialPort::Baud57600);
    port.setDataBits(QSerialPort::Data8);
    port.setParity(QSerialPort::NoParity);
    port.setFlowControl(QSerialPort::NoFlowControl);

    // Open the port - new settings should be applied
    QVERIFY(port.open(QIODevice::ReadOnly));
    // Close the port - the old settings should be restored
    port.close();

    PlatformSettings settingsAfterSecondClose(m_senderPortName);
    settingsAfterSecondClose.getPortSettings();
    QCOMPARE(settingsAfterSecondClose, settingsAfterClose);
    QCOMPARE_NE(settingsAfterSecondClose, initialSettings);

    // restore the original settings
    initialSettings.restorePortSettings();
#else
    QSKIP("macOS always resets the port on close.");
#endif
}

// The reader connects to readyRead() and then calls waitForReadyRead()
// inside the slot.
// The tst_QSerialPort::readyReadNotRecursive() test needs to make sure
// that the slot is entered only once.
class SemiBlockingReader : public QObject
{
    Q_OBJECT
public:
    explicit SemiBlockingReader(const QString &portName)
        : QObject(nullptr), m_portName(portName)
    {}

    int numSlotCalls() const { return m_numCalls; }
    QByteArray receivedData() const { return m_data; }

public slots:
    void start()
    {
        m_reader = new QSerialPort(m_portName, this);
        connect(m_reader, &QSerialPort::readyRead, this, &SemiBlockingReader::readData);
        if (!m_reader->open(QIODevice::ReadOnly))
            emit error();
    }

private slots:
    void readData()
    {
        ++m_numCalls;
        m_data.append(m_reader->readAll());
        while (m_reader->waitForReadyRead(1000))
            m_data.append(m_reader->readAll());
        emit allDataReceived();
    }

signals:
    void error();
    void allDataReceived();

private:
    QString m_portName;
    QByteArray m_data;
    QSerialPort *m_reader = nullptr;
    int m_numCalls = 0;
};

void tst_QSerialPort::readyReadNotRecursive()
{
    SemiBlockingReader reader(m_receiverPortName);
    QThread readerThread;
    connect(&readerThread, &QThread::started, &reader, &SemiBlockingReader::start);
    connect(&reader, &SemiBlockingReader::allDataReceived, &readerThread, &QThread::quit);
    int errorCount = 0;
    connect(&reader, &SemiBlockingReader::error, this, [&] {
        ++errorCount;
        readerThread.quit();
    });
    bool readerThreadFinished = false;
    connect(&readerThread, &QThread::finished, this, [&] { readerThreadFinished = true; });

    reader.moveToThread(&readerThread);
    readerThread.start();

    QSerialPort senderPort(m_senderPortName);
    QVERIFY(senderPort.open(QSerialPort::WriteOnly));
    [[maybe_unused]] AsyncWriterByTimer writer(senderPort, Qt::DirectConnection,
                                               alphabetArray, 50);

    QTRY_VERIFY(readerThreadFinished);
    QCOMPARE(errorCount, 0);
    QCOMPARE(reader.numSlotCalls(), 1);
    QCOMPARE(reader.receivedData(), alphabetArray);
}

void tst_QSerialPort::writeBufferSizeLimit_data()
{
    QTest::addColumn<qint64>("writeBufferSize");

    QTest::newRow("NoLimit") << 0LL;
    QTest::newRow("8Kb") << 8 * 1024LL;
}

void tst_QSerialPort::writeBufferSizeLimit()
{
    QFETCH(const qint64, writeBufferSize);

    auto setupPort = [](QSerialPort &port) {
        port.setBaudRate(QSerialPort::Baud115200);
        port.setDataBits(QSerialPort::Data8);
        port.setParity(QSerialPort::NoParity);
        port.setStopBits(QSerialPort::OneStop);
    };

    QSerialPort sender(m_senderPortName);
    QCOMPARE_EQ(sender.writeBufferSize(), 0); // unlimited by default
    QSignalSpy senderSpy(&sender, &QSerialPort::bytesWritten);
    setupPort(sender);
    sender.setWriteBufferSize(writeBufferSize);
    QCOMPARE_EQ(sender.writeBufferSize(), writeBufferSize);
    QVERIFY(sender.open(QIODevice::ReadWrite));

    QSerialPort receiver(m_receiverPortName);
    setupPort(receiver);
    QVERIFY(receiver.open(QIODevice::ReadWrite));

    static constexpr qsizetype DataSize = 16 * 1024;
    QVERIFY(DataSize > writeBufferSize); // to make sure that the test works correctly

    const QByteArray data(DataSize, 'a');

    qint64 written = 0;

    const bool inputBufferLimited = (writeBufferSize > 0);
    if (inputBufferLimited) {
        written = sender.write(data);
        QCOMPARE_EQ(written, writeBufferSize);
        // Writing again should return 0, as the buffer is full
        written = sender.write(data);
        QCOMPARE_EQ(written, 0); // can't add more

        // Wait until something is actually written to the device.
        // NOTE: If the test fails here due to a timeout, you might
        // want to use QTRY_VERIFY_WITH_TIMEOUT() and a larger interval.
        // The behavior can depend on a OS & driver combination!
        QTRY_VERIFY(senderSpy.size() > 0);
        qint64 reportedWritten = 0;
        for (const auto &val : std::as_const(senderSpy))
            reportedWritten += val.at(0).toULongLong();

        // Now we should be able to write at least reportedWritten bytes
        written = sender.write(data);
        QCOMPARE_GE(written, reportedWritten);
    } else {
        // Buffer is unlimited -> we should always write as much as we can
        written = sender.write(data);
        QCOMPARE_EQ(written, data.size());

        written = sender.write(data);
        QCOMPARE_EQ(written, data.size());

        // Wait until something is actually written to the device.
        // NOTE: If the test fails here due to a timeout, you might
        // want to use QTRY_VERIFY_WITH_TIMEOUT() and a larger interval.
        // The behavior can depend on a OS & driver combination!
        QTRY_VERIFY(senderSpy.size() > 0);

        written = sender.write(data);
        QCOMPARE_EQ(written, data.size());
    }

    receiver.close();
    sender.close();
}

QTEST_MAIN(tst_QSerialPort)
#include "tst_qserialport.moc"
