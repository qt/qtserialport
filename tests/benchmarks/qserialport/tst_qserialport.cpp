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

class tst_QSerialPort : public QObject
{
    Q_OBJECT
public:
    explicit tst_QSerialPort();

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

private slots:
    void initTestCase();

    void synchronousLoopbackDataVerificationTest();

private:
#ifdef Q_OS_WIN
    void clearReceiver();
#endif

    QString m_senderPortName;
    QString m_receiverPortName;

    static int loopLevel;
};

int tst_QSerialPort::loopLevel = 0;

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
    }
}

void tst_QSerialPort::synchronousLoopbackDataVerificationTest()
{
#ifdef Q_OS_WIN
    clearReceiver();
#endif

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

    QCOMPARE(writeData, readData);
}

QTEST_MAIN(tst_QSerialPort)
#include "tst_qserialport.moc"
