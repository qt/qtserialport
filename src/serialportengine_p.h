/*
    License...
*/

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
