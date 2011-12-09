/*
    License...
*/

#ifndef SERIALPORTENGINE_P_WIN_H
#define SERIALPORTENGINE_P_WIN_H

#include "serialport.h"
#include "serialportengine_p.h"

#include <qt_windows.h>
#if defined (Q_OS_WINCE)
#  include <QtCore/qmutex.h>
#  include <QtCore/qthread.h>
#  include <QtCore/qtimer.h>
#else
#  include <QtCore/private/qwineventnotifier_p.h>
#endif

QT_BEGIN_NAMESPACE

#if defined (Q_OS_WINCE)

class WinCeWaitCommEventBreaker : public QThread
{
    Q_OBJECT
public:
    WinCeWaitCommEventBreaker(HANDLE descriptor, int timeout, QObject *parent = 0)
        : QThread(parent), m_descriptor(descriptor),
          m_timeout(timeout), m_worked(false) {
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
    WinSerialPortEngine(SerialPortPrivate *parent);
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

    virtual bool processIOErrors();

#if defined (Q_OS_WINCE)
    virtual void lockNotification(NotificationLockerType type, bool uselocker);
    virtual void unlockNotification(NotificationLockerType type);
#endif

protected:
    virtual void detectDefaultSettings();

#if defined (Q_OS_WINCE)
    virtual void run();
#else
    virtual bool event(QEvent *e);
#endif

private:
    DCB m_currDCB;
    DCB m_oldDCB;
    COMMTIMEOUTS m_currCommTimeouts;
    COMMTIMEOUTS m_oldCommTimeouts;
    HANDLE m_descriptor;
    bool m_flagErrorFromCommEvent;
    DWORD m_currentMask;
    DWORD m_setMask;

#if defined (Q_OS_WINCE)
    QMutex m_readNotificationMutex;
    QMutex m_writeNotificationMutex;
    QMutex m_errorNotificationMutex;
    QMutex m_settingsChangeMutex;
    QMutex m_setCommMaskMutex;
    volatile bool m_running;
#else
    OVERLAPPED m_ovRead;
    OVERLAPPED m_ovWrite;
    OVERLAPPED m_ovSelect;
    OVERLAPPED m_ov;

    bool createEvents(bool rx, bool tx);
    void closeEvents();
    void setMaskAndActivateEvent();
#endif

    bool updateDcb();
    bool updateCommTimeouts();
};

QT_END_NAMESPACE

#endif // SERIALPORTENGINE_P_WIN_H
