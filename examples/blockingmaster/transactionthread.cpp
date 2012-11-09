/****************************************************************************
**
** Copyright (C) 2012 Denis Shienkov <scapig@yandex.ru>
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

#include "transactionthread.h"

#include <serialport.h>

#include <QTime>

QT_USE_NAMESPACE_SERIALPORT

TransactionThread::TransactionThread(QObject *parent)
    : QThread(parent), waitTimeout(0), quit(false)
{
}

TransactionThread::~TransactionThread()
{
    mutex.lock();
    quit = true;
    cond.wakeOne();
    mutex.unlock();
    wait();
}

void TransactionThread::startNewMaster(const QString &port, int transactionWaitTimeout, const QString &request)
{
    QMutexLocker locker(&mutex);
    portName = port;
    waitTimeout = transactionWaitTimeout;
    requestText = request;

    if (!isRunning())
        start();
    else
        cond.wakeOne();
}

void TransactionThread::run()
{
    bool currentSerialPortNameChanged = false;

    mutex.lock();
    QString currentSerialPortName;
    if (currentSerialPortName != portName) {
        currentSerialPortName = portName;
        currentSerialPortNameChanged = true;
    }

    int currentTransactionWaitTimeout = waitTimeout;
    QString currentRequestText = requestText;
    mutex.unlock();

    SerialPort serial;

    while (!quit) {

        if (currentSerialPortNameChanged) {
            serial.close();
            serial.setPort(currentSerialPortName);

            if (!serial.open(QIODevice::ReadWrite)) {
                emit error(tr("Can't open %1, error code %2")
                           .arg(portName).arg(serial.error()));
                return;
            }

            if (!serial.setRate(9600)) {
                emit error(tr("Can't set rate 9600 baud to port %1, error code %2")
                           .arg(portName).arg(serial.error()));
                return;
            }

            if (!serial.setDataBits(SerialPort::Data8)) {
                emit error(tr("Can't set 8 data bits to port %1, error code %2")
                           .arg(portName).arg(serial.error()));
                return;
            }

            if (!serial.setParity(SerialPort::NoParity)) {
                emit error(tr("Can't set no patity to port %1, error code %2")
                           .arg(portName).arg(serial.error()));
                return;
            }

            if (!serial.setStopBits(SerialPort::OneStop)) {
                emit error(tr("Can't set 1 stop bit to port %1, error code %2")
                           .arg(portName).arg(serial.error()));
                return;
            }

            if (!serial.setFlowControl(SerialPort::NoFlowControl)) {
                emit error(tr("Can't set no flow control to port %1, error code %2")
                           .arg(portName).arg(serial.error()));
                return;
            }
        }

        // write request
        QByteArray requestData = currentRequestText.toLocal8Bit();
        serial.write(requestData);
        if (serial.waitForBytesWritten(waitTimeout)) {

            // read response
            if (serial.waitForReadyRead(currentTransactionWaitTimeout)) {
                QByteArray responseData = serial.readAll();
                while (serial.waitForReadyRead(10))
                    responseData += serial.readAll();

                QString responseText(responseData);
                emit response(responseText);
            } else {
                emit timeout(tr("Wait read response timeout %1")
                             .arg(QTime::currentTime().toString()));
            }
        } else {
            emit timeout(tr("Wait write request timeout %1")
                         .arg(QTime::currentTime().toString()));
        }

        mutex.lock();
        cond.wait(&mutex);
        if (currentSerialPortName != portName) {
            currentSerialPortName = portName;
            currentSerialPortNameChanged = true;
        } else {
            currentSerialPortNameChanged = false;
        }
        currentTransactionWaitTimeout = waitTimeout;
        currentRequestText = requestText;
        mutex.unlock();
    }
}
