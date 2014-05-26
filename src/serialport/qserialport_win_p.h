/****************************************************************************
**
** Copyright (C) 2012 Denis Shienkov <denis.shienkov@gmail.com>
** Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
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

#ifndef QSERIALPORT_WIN_P_H
#define QSERIALPORT_WIN_P_H

#include "qserialport_p.h"

#include <QtCore/qhash.h>

#include <qt_windows.h>

QT_BEGIN_NAMESPACE

class QWinEventNotifier;
class QTimer;

class QSerialPortPrivate : public QSerialPortPrivateData
{
    Q_DECLARE_PUBLIC(QSerialPort)

public:
    QSerialPortPrivate(QSerialPort *q);

    bool open(QIODevice::OpenMode mode);
    void close();

    QSerialPort::PinoutSignals pinoutSignals();

    bool setDataTerminalReady(bool set);
    bool setRequestToSend(bool set);

    bool flush();
    bool clear(QSerialPort::Directions directions);

    bool sendBreak(int duration);
    bool setBreakEnabled(bool set);

    void startWriting();

    bool waitForReadyRead(int msec);
    bool waitForBytesWritten(int msec);

    bool setBaudRate();
    bool setBaudRate(qint32 baudRate, QSerialPort::Directions directions);
    bool setDataBits(QSerialPort::DataBits dataBits);
    bool setParity(QSerialPort::Parity parity);
    bool setStopBits(QSerialPort::StopBits stopBits);
    bool setFlowControl(QSerialPort::FlowControl flowControl);
    bool setDataErrorPolicy(QSerialPort::DataErrorPolicy policy);

    void handleLineStatusErrors();
    QSerialPort::SerialPortError decodeSystemError() const;

    void _q_completeAsyncCommunication();
    void _q_completeAsyncRead();
    void _q_completeAsyncWrite();

    bool startAsyncCommunication();
    bool startAsyncRead();
    bool startAsyncWrite();

    bool emulateErrorPolicy();
    void emitReadyRead();

    static QString portNameToSystemLocation(const QString &port);
    static QString portNameFromSystemLocation(const QString &location);

    static qint32 baudRateFromSetting(qint32 setting);
    static qint32 settingFromBaudRate(qint32 baudRate);

    static QList<qint32> standardBaudRates();

    DCB currentDcb;
    DCB restoredDcb;
    COMMTIMEOUTS currentCommTimeouts;
    COMMTIMEOUTS restoredCommTimeouts;
    HANDLE handle;
    bool parityErrorOccurred;
    QByteArray readChunkBuffer;
    bool readyReadEmitted;
    bool writeStarted;
    QWinEventNotifier *communicationNotifier;
    QWinEventNotifier *readCompletionNotifier;
    QWinEventNotifier *writeCompletionNotifier;
    QTimer *startAsyncWriteTimer;
    OVERLAPPED communicationOverlapped;
    OVERLAPPED readCompletionOverlapped;
    OVERLAPPED writeCompletionOverlapped;
    DWORD originalEventMask;
    DWORD triggeredEventMask;

private:
    bool updateDcb();
    bool updateCommTimeouts();


    bool waitAnyEvent(int msecs, bool *timedOut, HANDLE *triggeredEvent);

};

QT_END_NAMESPACE

#endif // QSERIALPORT_WIN_P_H
