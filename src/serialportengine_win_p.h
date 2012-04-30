/****************************************************************************
**
** Copyright (C) 2012 Denis Shienkov <scapig@yandex.ru>
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

#ifndef SERIALPORTENGINE_WIN_P_H
#define SERIALPORTENGINE_WIN_P_H

#include "serialport.h"
#include "serialportengine_p.h"

#include <qt_windows.h>
#if defined (Q_OS_WINCE)
#  include <QtCore/qmutex.h>
#  include <QtCore/qthread.h>
#  include <QtCore/qtimer.h>
#else
# if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#  include <QtCore/qwineventnotifier.h>
# else
#  include "qt4support/qwineventnotifier_p.h"
# endif
#endif

QT_BEGIN_NAMESPACE_SERIALPORT

#if defined (Q_OS_WINCE)

class WinCeWaitCommEventBreaker : public QThread
{
    Q_OBJECT
public:
    WinCeWaitCommEventBreaker(HANDLE descriptor, int timeout, QObject *parent = 0)
        : QThread(parent)
        , m_descriptor(descriptor)
        , m_timeout(timeout)
        , m_worked(false) {
        start();
    }
    virtual ~WinCeWaitCommEventBreaker() {
        stop();
        wait();
    }
    void stop() { exit(0); }
    bool isWorked() const { return m_worked; }

protected:
    void run() {
        QTimer timer;
        QObject::connect(&timer, SIGNAL(timeout()), this, SLOT(procTimeout()), Qt::DirectConnection);
        timer.start(m_timeout);
        exec();
        m_worked = true;
    }

private slots:
    void procTimeout() {
        ::SetCommMask(m_descriptor, 0);
        stop();
    }

private:
    HANDLE m_descriptor;
    int m_timeout;
    volatile bool m_worked;
};
#endif

#if defined (Q_OS_WINCE)
class WinSerialPortEngine : public QThread, public SerialPortEngine
#else
class WinSerialPortEngine : public QWinEventNotifier, public SerialPortEngine
#endif
{
    Q_OBJECT
public:
    WinSerialPortEngine(SerialPortPrivate *d);
    virtual ~WinSerialPortEngine();

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

#if defined (Q_OS_WINCE)
    // FIXME
    virtual void lockNotification(NotificationLockerType type, bool uselocker);
    virtual void unlockNotification(NotificationLockerType type);
#endif

public:
    static QList<qint32> standardRates();

protected:
    virtual void detectDefaultSettings();

#if defined (Q_OS_WINCE)
    virtual void run();
#else
    virtual bool event(QEvent *e);
#endif

private:

#if !defined (Q_OS_WINCE)
    bool createEvents(bool rx, bool tx);
    void closeEvents();
#endif

    bool isNotificationEnabled(DWORD mask) const;
    void setNotificationEnabled(bool enable, DWORD mask);

    bool updateDcb();
    bool updateCommTimeouts();

private:
    DCB m_currentDcb;
    DCB m_restoredDcb;
    COMMTIMEOUTS m_currentCommTimeouts;
    COMMTIMEOUTS m_restoredCommTimeouts;
    HANDLE m_descriptor;
    bool m_flagErrorFromCommEvent;
    DWORD m_currentMask;
    DWORD m_desiredMask;

#if defined (Q_OS_WINCE)
    QMutex m_readNotificationMutex;
    QMutex m_writeNotificationMutex;
    QMutex m_errorNotificationMutex;
    QMutex m_settingsChangeMutex;
    QMutex m_setCommMaskMutex;
    volatile bool m_running;
#else
    OVERLAPPED m_readOverlapped;
    OVERLAPPED m_writeOverlapped;
    OVERLAPPED m_selectOverlapped;
    OVERLAPPED m_notifyOverlapped;
#endif
};

QT_END_NAMESPACE_SERIALPORT

#endif // SERIALPORTENGINE_WIN_P_H
