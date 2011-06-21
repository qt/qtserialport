/*
    License...
*/

#ifndef SERIALPORT_P_UNIX_H
#define SERIALPORT_P_UNIX_H

#include "serialport_p.h"

#include <termios.h>
//#  include "../unix/ttylocker.h"


class SerialPortPrivateUnix : public SerialPortPrivate
{
public:
    SerialPortPrivateUnix();
    virtual ~SerialPortPrivateUnix();

    virtual bool open(QIODevice::OpenMode mode);
    virtual void close();

    virtual bool dtr() const;
    virtual bool rts() const;

    virtual SerialPort::Lines lines() const;

    virtual bool flush();
    virtual bool reset();

    virtual qint64 bytesAvailable() const;

    virtual qint64 read(char *data, qint64 len);
    virtual qint64 write(const char *data, qint64 len);

    virtual bool waitForReadyRead(int msec);
    virtual bool waitForBytesWritten(int msec);

protected:
    virtual bool setNativeRate(int rate, SerialPort::Directions dir);
    virtual bool setNativeDataBits(SerialPort::DataBits dataBits);
    virtual bool setNativeParity(SerialPort::Parity parity);
    virtual bool setNativeStopBits(SerialPort::StopBits stopBits);
    virtual bool setNativeFlowControl(SerialPort::FlowControl flowControl);
    virtual bool setNativeDataErrorPolicy(SerialPort::DataErrorPolicy policy);

    virtual void detectDefaultSettings();

    virtual bool saveOldsettings();
    virtual bool restoreOldsettings();

private:
    struct termios mTermios;
    struct termios mOldTermios;
    void prepareTimeouts(int msecs);
    bool updateTermious();
};

#endif // SERIALPORT_P_UNIX_H
