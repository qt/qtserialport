/*
    License...
*/

#ifndef SERIALPORT_P_WIN_H
# define SERIALPORT_P_WIN_H

# include "serialport_p.h"

# include <qt_windows.h>


class SerialPortPrivateWin : public SerialPortPrivate
{
public:
    SerialPortPrivateWin();
    virtual ~SerialPortPrivateWin();

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
    enum CommTimeouts {
        ReadIntervalTimeout, ReadTotalTimeoutMultiplier,
        ReadTotalTimeoutConstant, WriteTotalTimeoutMultiplier,
        WriteTotalTimeoutConstant
    };

    DCB mDCB, mOldDCB;
    COMMTIMEOUTS mCommTimeouts, mOldCommTimeouts;
    OVERLAPPED mOvRead;
    OVERLAPPED mOvWrite;
    OVERLAPPED mOvSelect;

    bool createEvents(bool rx, bool tx);
    bool closeEvents() const;
    void recalcTotalReadTimeoutConstant();
    void prepareCommTimeouts(CommTimeouts cto, DWORD msecs);
    bool updateDcb();
    bool updateCommTimeouts();
};

#endif // SERIALPORT_P_WIN_H
