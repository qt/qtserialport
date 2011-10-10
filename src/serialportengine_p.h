/*
    License...
*/

#ifndef SERIALPORTENGINE_P_H
#define SERIALPORTENGINE_P_H

#include "serialport.h"
#include "serialport_p.h"

QT_BEGIN_NAMESPACE

class SerialPortNotifier
{
public:
    virtual bool isReadNotificationEnabled() const = 0;
    virtual void setReadNotificationEnabled(bool enable) = 0;
    virtual bool isWriteNotificationEnabled() const = 0;
    virtual void setWriteNotificationEnabled(bool enable) = 0;

protected:
    SerialPortPrivate *m_parent;
};

class SerialPortEngine : public SerialPortNotifier
{
public:
    static SerialPortEngine *create(SerialPortPrivate *parent);

    virtual bool nativeOpen(const QString &location, QIODevice::OpenMode mode) = 0;
    virtual void nativeClose(const QString &location) = 0;

    virtual SerialPort::Lines nativeLines() const = 0;

    virtual bool setNativeDtr(bool set) = 0;
    virtual bool setNativeRts(bool set) = 0;

    virtual bool nativeFlush() = 0;
    virtual bool nativeReset() = 0;

    virtual bool nativeSendBreak(int duration) = 0;
    virtual bool nativeSetBreak(bool set) = 0;

    virtual qint64 nativeBytesAvailable() const = 0;
    virtual qint64 nativeBytesToWrite() const = 0;

    virtual qint64 nativeRead(char *data, qint64 len) = 0;
    virtual qint64 nativeWrite(const char *data, qint64 len) = 0;
    virtual bool nativeSelect(int timeout,
                              bool checkRead, bool checkWrite,
                              bool *selectForRead, bool *selectForWrite) = 0;

    virtual QString nativeToSystemLocation(const QString &port) const = 0;
    virtual QString nativeFromSystemLocation(const QString &location) const = 0;

    virtual bool setNativeRate(qint32 rate, SerialPort::Directions dir) = 0;
    virtual bool setNativeDataBits(SerialPort::DataBits dataBits) = 0;
    virtual bool setNativeParity(SerialPort::Parity parity) = 0;
    virtual bool setNativeStopBits(SerialPort::StopBits stopBits) = 0;
    virtual bool setNativeFlowControl(SerialPort::FlowControl flowControl) = 0;

    virtual bool setNativeDataInterval(int usecs) = 0;
    virtual bool setNativeReadTimeout(int msecs) = 0;

    virtual bool setNativeDataErrorPolicy(SerialPort::DataErrorPolicy policy) = 0;

    virtual bool isReadNotificationEnabled() const = 0;
    virtual void setReadNotificationEnabled(bool enable) = 0;
    virtual bool isWriteNotificationEnabled() const = 0;
    virtual void setWriteNotificationEnabled(bool enable) = 0;

    virtual bool processNativeIOErrors() = 0;

protected:
    bool m_oldSettingsIsSaved;

    virtual void detectDefaultSettings() = 0;
    virtual bool saveOldsettings() = 0;
    virtual bool restoreOldsettings() = 0;
    virtual void prepareOtherOptions() = 0;
};

QT_END_NAMESPACE

#endif // SERIALPORTENGINE_P_H
