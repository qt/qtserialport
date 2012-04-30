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

#ifndef SERIALPORTENGINE_UNIX_P_H
#define SERIALPORTENGINE_UNIX_P_H

#include "serialport.h"
#include "serialportengine_p.h"

#include <termios.h>
#if defined (Q_OS_LINUX)
#  include <linux/serial.h>
#endif

class QSocketNotifier;

QT_BEGIN_NAMESPACE_SERIALPORT

class UnixSerialPortEngine : public QObject, public SerialPortEngine
{
    Q_OBJECT
public:
    UnixSerialPortEngine(SerialPortPrivate *d);
    virtual ~UnixSerialPortEngine();

    virtual bool open(const QString &location, QIODevice::OpenMode mode);
    virtual void close(const QString &location);

    virtual SerialPort::Lines lines() const;

    virtual bool setDtr(bool set);
    virtual bool setRts(bool set);

    virtual bool flush();
    virtual bool reset();

    virtual bool sendBreak(int duration);
    virtual bool setBreak(bool set);

    virtual qint64 bytesAvailable() const;
    virtual qint64 bytesToWrite() const;

    virtual qint64 read(char *data, qint64 len);
    virtual qint64 write(const char *data, qint64 len);
    virtual bool select(int timeout,
                        bool checkRead, bool checkWrite,
                        bool *selectForRead, bool *selectForWrite);

    virtual QString toSystemLocation(const QString &port) const;
    virtual QString fromSystemLocation(const QString &location) const;

    virtual bool setRate(qint32 rate, SerialPort::Directions dir);
    virtual bool setDataBits(SerialPort::DataBits dataBits);
    virtual bool setParity(SerialPort::Parity parity);
    virtual bool setStopBits(SerialPort::StopBits stopBits);
    virtual bool setFlowControl(SerialPort::FlowControl flowControl);

    virtual bool setDataErrorPolicy(SerialPort::DataErrorPolicy policy);

    virtual bool isReadNotificationEnabled() const;
    virtual void setReadNotificationEnabled(bool enable);
    virtual bool isWriteNotificationEnabled() const;
    virtual void setWriteNotificationEnabled(bool enable);
    virtual bool isErrorNotificationEnabled() const;
    virtual void setErrorNotificationEnabled(bool enable);

    virtual bool processIOErrors();

public:
    static qint32 rateFromSetting(qint32 setting);
    static qint32 settingFromRate(qint32 rate);
    static QList<qint32> standardRates();

protected:
    virtual void detectDefaultSettings();
    virtual bool eventFilter(QObject *obj, QEvent *e);

private:
    bool updateTermios();

#if !defined (CMSPAR)
    qint64 writePerChar(const char *data, qint64 maxSize);
#endif
    qint64 readPerChar(char *data, qint64 maxSize);

private:
    struct termios m_currentTermios;
    struct termios m_restoredTermios;
#if defined (Q_OS_LINUX)
    struct serial_struct m_currentSerialInfo;
    struct serial_struct m_restoredSerialInfo;
#endif
    int m_descriptor;
    bool m_isCustomRateSupported;

    QSocketNotifier *m_readNotifier;
    QSocketNotifier *m_writeNotifier;
    QSocketNotifier *m_exceptionNotifier;
};

QT_END_NAMESPACE_SERIALPORT

#endif // SERIALPORTENGINE_UNIX_P_H
