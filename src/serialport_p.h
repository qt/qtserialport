/****************************************************************************
**
** Copyright (C) 2011-2012 Denis Shienkov <scapig2@yandex.ru>
** Copyright (C) 2011 Sergey Belyashov <Sergey.Belyashov@gmail.com>
** Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
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

#ifndef SERIALPORT_P_H
#define SERIALPORT_P_H

#include "serialport.h"

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <private/qringbuffer_p.h>
#else
#include "qt4support/qringbuffer_p.h"
#endif

QT_BEGIN_NAMESPACE_SERIALPORT

class SerialPortEngine;

// General port parameters (for any OS).
class SerialPortOptions
{
public:
    SerialPortOptions()
        : inputRate(SerialPort::UnknownRate)
        , outputRate(SerialPort::UnknownRate)
        , dataBits(SerialPort::UnknownDataBits)
        , parity(SerialPort::UnknownParity)
        , stopBits(SerialPort::UnknownStopBits)
        , flow(SerialPort::UnknownFlowControl)
        , policy(SerialPort::IgnorePolicy)
        , restoreSettingsOnClose(true)
    {}

    QString systemLocation;
    qint32 inputRate;
    qint32 outputRate;
    SerialPort::DataBits dataBits;
    SerialPort::Parity parity;
    SerialPort::StopBits stopBits;
    SerialPort::FlowControl flow;
    SerialPort::DataErrorPolicy policy;
    bool restoreSettingsOnClose;
};

class SerialPortPrivate
{
    Q_DECLARE_PUBLIC(SerialPort)
public:
    SerialPortPrivate(SerialPort *parent);
    virtual ~SerialPortPrivate();

    void setPort(const QString &port);
    QString port() const;

    bool open(QIODevice::OpenMode mode);
    void close();

    bool setRate(qint32 rate, SerialPort::Directions dir);
    qint32 rate(SerialPort::Directions dir) const;

    bool setDataBits(SerialPort::DataBits dataBits);
    SerialPort::DataBits dataBits() const;

    bool setParity(SerialPort::Parity parity);
    SerialPort::Parity parity() const;

    bool setStopBits(SerialPort::StopBits stopBits);
    SerialPort::StopBits stopBits() const;

    bool setFlowControl(SerialPort::FlowControl flow);
    SerialPort::FlowControl flowControl() const;

    bool dtr() const;
    bool rts() const;

    bool setDtr(bool set);
    bool setRts(bool set);

    SerialPort::Lines lines() const;

    bool flush();
    bool reset();

    bool sendBreak(int duration);
    bool setBreak(bool set);

    bool setDataErrorPolicy(SerialPort::DataErrorPolicy policy);
    SerialPort::DataErrorPolicy dataErrorPolicy() const;

    SerialPort::PortError error() const;
    void unsetError();
    void setError(SerialPort::PortError error);

    qint64 bytesAvailable() const;
    qint64 bytesToWrite() const;

    qint64 read(char *data, qint64 len);
    qint64 write(const char *data, qint64 len);
    bool waitForReadOrWrite(int timeout,
                            bool checkRead, bool checkWrite,
                            bool *selectForRead, bool *selectForWrite);

    void clearBuffers();
    bool readFromPort();

    bool canReadNotification();
    bool canWriteNotification();
    bool canErrorNotification();

public:
    qint64 readBufferMaxSize;
    QRingBuffer readBuffer;
    QRingBuffer writeBuffer;
    bool isBuffered;

    bool readSerialNotifierCalled;
    bool readSerialNotifierState;
    bool readSerialNotifierStateSet;
    bool emittedReadyRead;
    bool emittedBytesWritten;

    SerialPort::PortError portError;

    SerialPortEngine *engine;

    SerialPortOptions options;

private:
    SerialPort * const q_ptr;
};

QT_END_NAMESPACE_SERIALPORT

#endif // SERIALPORT_P_H
