/*
    License...
*/

#ifndef SERIALPORT_P_H
#define SERIALPORT_P_H

#include "serialport.h"
#include "abstractserialport_p.h"

#if defined (Q_OS_WIN)
#  include <qt_windows.h>
#else
#  include <termios.h>
#endif

class SerialPortPrivate : public AbstractSerialPortPrivate
{
    Q_DECLARE_PUBLIC(SerialPort)
public:
    SerialPortPrivate();

    virtual bool open(QIODevice::OpenMode mode);
    virtual void close();

    virtual SerialPort::Lines lines() const;

    virtual bool flush();
    virtual bool reset();

    virtual qint64 bytesAvailable() const;

    virtual qint64 read(char *data, qint64 len);
    virtual qint64 write(const char *data, qint64 len);
    virtual bool waitForReadOrWrite(int timeout,
                                    bool checkRead, bool checkWrite,
                                    bool *selectForRead, bool *selectForWrite);

#if defined (Q_OS_WIN)
    HANDLE descriptor() { return m_descriptor; }
#else
    int descriptor() { return m_descriptor; }
#endif

    inline void readNotification() { canReadNotification(); }
    inline void writeNotification() { canWriteNotification(); }

protected:
    virtual QString nativeToSystemLocation(const QString &port) const;
    virtual QString nativeFromSystemLocation(const QString &location) const;

    virtual bool setNativeRate(qint32 rate, SerialPort::Directions dir);
    virtual bool setNativeDataBits(SerialPort::DataBits dataBits);
    virtual bool setNativeParity(SerialPort::Parity parity);
    virtual bool setNativeStopBits(SerialPort::StopBits stopBits);
    virtual bool setNativeFlowControl(SerialPort::FlowControl flowControl);

    virtual bool setNativeDataInterval(int usecs);
    virtual bool setNativeReadTimeout(int msecs);

    virtual bool setNativeDataErrorPolicy(SerialPort::DataErrorPolicy policy);

    virtual void detectDefaultSettings();

    virtual bool saveOldsettings();
    virtual bool restoreOldsettings();

private:
#if defined (Q_OS_WIN)
    enum CommTimeouts {
        ReadIntervalTimeout, ReadTotalTimeoutMultiplier,
        ReadTotalTimeoutConstant, WriteTotalTimeoutMultiplier,
        WriteTotalTimeoutConstant
    };

    DCB m_currDCB, m_oldDCB;
    COMMTIMEOUTS m_currCommTimeouts, m_oldCommTimeouts;
    OVERLAPPED m_ovRead;
    OVERLAPPED m_ovWrite;
    OVERLAPPED m_ovSelect;

    HANDLE m_descriptor;

    bool createEvents(bool rx, bool tx);
    void closeEvents() const;
    void recalcTotalReadTimeoutConstant();
    void prepareCommTimeouts(CommTimeouts cto, DWORD msecs);
    bool updateDcb();
    bool updateCommTimeouts();

    bool isRestrictedAreaSettings(SerialPort::DataBits dataBits,
                                  SerialPort::StopBits stopBits) const;
    void prepareOtherOptions();
#else
    struct termios m_currTermios;
    struct termios m_oldTermios;

    int m_descriptor;

    void prepareTimeouts(int msecs);
    bool updateTermious();
    bool setStandartRate(SerialPort::Directions dir, speed_t speed);
    bool setCustomRate(qint32 speed);
    friend class SerialPortNotifier;
#endif

    //
    SerialPort *q_ptr;

    bool canReadNotification();
    bool canWriteNotification();

    bool isReadNotificationEnabled() const;
    void setReadNotificationEnabled(bool enable, bool onClose = false);
    bool isWriteNotificationEnabled() const;
    void setWriteNotificationEnabled(bool enable, bool onClose = false);

    void clearNotification();

    void clearBuffers();
    bool readFromSerial();
};

#endif // SERIALPORT_P_H
