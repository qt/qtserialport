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

#ifndef SERIALPORTENGINE_P_H
#define SERIALPORTENGINE_P_H

#include "serialport.h"
#include "serialport_p.h"

QT_BEGIN_NAMESPACE_SERIALPORT

class SerialPortEngine
{
public:

#if defined (Q_OS_WINCE)
    // FIXME
    // Only for WinCE.
    enum NotificationLockerType {
        CanReadLocker,
        CanWriteLocker,
        CanErrorLocker
    };
#endif

    virtual ~SerialPortEngine() {}

    static SerialPortEngine *create(SerialPortPrivate *d);

    virtual bool open(const QString &location, QIODevice::OpenMode mode) = 0;
    virtual void close(const QString &location) = 0;

    virtual SerialPort::Lines lines() const = 0;

    virtual bool setDtr(bool set) = 0;
    virtual bool setRts(bool set) = 0;

    virtual bool flush() = 0;
    virtual bool reset() = 0;

    virtual bool sendBreak(int duration) = 0;
    virtual bool setBreak(bool set) = 0;

    virtual qint64 bytesAvailable() const = 0;
    virtual qint64 bytesToWrite() const = 0;

    virtual qint64 read(char *data, qint64 len) = 0;
    virtual qint64 write(const char *data, qint64 len) = 0;
    virtual bool select(int timeout,
                        bool checkRead, bool checkWrite,
                        bool *selectForRead, bool *selectForWrite) = 0;

    virtual QString toSystemLocation(const QString &port) const = 0;
    virtual QString fromSystemLocation(const QString &location) const = 0;

    virtual bool setRate(qint32 rate, SerialPort::Directions dir) = 0;
    virtual bool setDataBits(SerialPort::DataBits dataBits) = 0;
    virtual bool setParity(SerialPort::Parity parity) = 0;
    virtual bool setStopBits(SerialPort::StopBits stopBits) = 0;
    virtual bool setFlowControl(SerialPort::FlowControl flowControl) = 0;

    virtual bool setDataErrorPolicy(SerialPort::DataErrorPolicy policy) = 0;

    virtual bool isReadNotificationEnabled() const = 0;
    virtual void setReadNotificationEnabled(bool enable) = 0;
    virtual bool isWriteNotificationEnabled() const = 0;
    virtual void setWriteNotificationEnabled(bool enable) = 0;
    virtual bool isErrorNotificationEnabled() const = 0;
    virtual void setErrorNotificationEnabled(bool enable) = 0;

    virtual bool processIOErrors() = 0;

#if defined (Q_OS_WINCE)
    // FIXME
    virtual void lockNotification(NotificationLockerType type, bool uselocker) = 0;
    virtual void unlockNotification(NotificationLockerType type) = 0;
#endif

protected:
    virtual void detectDefaultSettings() = 0;

protected:
    SerialPortPrivate *dptr;
};

QT_END_NAMESPACE_SERIALPORT

#endif // SERIALPORTENGINE_P_H
