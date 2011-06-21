/*
    License...
*/

#ifndef SERIALPORT_P_H
# define SERIALPORT_P_H

# include "serialport.h"

class SerialPortPrivate
{
public:
    SerialPortPrivate()
        : mInRate(SerialPort::UnknownRate)
        , mOutRate(SerialPort::UnknownRate)
        , mDataBits(SerialPort::UnknownDataBits)
        , mParity(SerialPort::UnknownParity)
        , mStopBits(SerialPort::UnknownStopBits)
        , mFlow(SerialPort::UnknownFlowControl)
        , mDataInterval(0)
        , mReadTimeout(0)
        , mPolicy(SerialPort::IgnorePolicy)
        , mError(SerialPort::NoError)
    {}

    void setPort(const QString &port) { mPort = port; }
    const QString& port() const { return mPort; }

    virtual bool open(QIODevice::OpenMode mode) = 0;
    virtual void close() = 0;

    bool setRate(qint32 rate, SerialPort::Directions dir) {
        if (setNativeRate(rate, dir)) {
            mRate = rate;
            return true;
        }
        return false;
    }
    qint32 rate(SerialPort::Directions dir) const {
        if (dir & Input)
            return (dir & Output) ? ((mInRate + mOutRate)/2) : mInRate;
        return mOutRate;
    }
    bool setDataBits(SerialPort::DataBits dataBits) {
        if (setNativeDataBits(dataBits)) {
            mDataBits = dataBits;
            return true;
        }
        return false;
    }
    SerialPort::DataBits dataBits() const { return mDataBits; }

    bool setParity(SerialPort::Parity parity) {
        if (setNativeParity(parity)) {
            mParity = parity;
            return true;
        }
        return false;
    }
    SerialPort::Parity parity() const { return mParity; }

    bool setStopBits(SerialPort::StopBits stopBits) {
        if (setNativeStopBits(stopBits)) {
            mStopBits = stopBits;
            return true;
        }
        return false;
    }
    SerialPort::StopBits stopBits() const { return mStopBits; }

    bool setFlowControl(SerialPort::FlowControl flow) {
        if (setNativeFlowControl(flow)) {
            mFlow = flow;
            return true;
        }
        return false;
    }
    SerialPort::FlowControl flowControl() const { return mFlow; }

    void setDataInterval(int usecs) {
        if (setNativeDataInterval(usecs)) {
            mDataInterval = usecs;
            return true;
        }
        return false;
    }
    int dataInterval() const { return mDataInterval; }

    void setReadTimeout(int msecs) {
        if (setNativeReadTimeout(msecs)) {
            mReadTimeout = msecs;
            return true;
        }
        return false;
    }
    int readTimeout() const { return mReadTimeout; }

    virtual bool dtr() const = 0;
    virtual bool rts() const = 0;

    virtual SerialPort::Lines lines() const = 0;

    virtual bool flush() = 0;
    virtual bool reset() = 0;

    void setDataErrorPolicy(SerialPort::DataErrorPolicy policy) {
        if (setNativeDataErrorPolicy(policy)) {
            mPolicy = policy;
            return true;
        }
        return false;
    }
    SerialPort::DataErrorPolicy dataErrorPolicy() const { return mPolicy; }

    SerialPort::PortError error() const { return mError; }
    void unsetError() { mError = SerialPort::NoError; }

    void setError(SerialPort::PortError error) { mError = error; }

    virtual qint64 bytesAvailable() const = 0;

    virtual qint64 read(char *data, qint64 len) = 0;
    virtual qint64 write(const char *data, qint64 len) = 0;
    virtual bool waitForReadyRead(int msec) = 0;
    virtual bool waitForBytesWritten(int msec) = 0;
protected:
    //General (for any OS) private parameters
    QString mPort;
    qint32 mInRate;
    qint32 mOutRate;
    SerialPort::DataBits mDataBits;
    SerialPort::Parity mParity;
    SerialPort::StopBits mStopBits;
    SerialPort::FlowControl mFlow;
    int mDataInterval;
    int mReadTimeout;
    SerialPort::DataErrorPolicy mPolicy;
    SerialPort::PortError mError;

    virtual bool setNativeRate(int rate, SerialPort::Directions dir) = 0;
    virtual bool setNativeDataBits(SerialPort::DataBits dataBits) = 0;
    virtual bool setNativeParity(SerialPort::Parity parity) = 0;
    virtual bool setNativeStopBits(SerialPort::StopBits stopBits) = 0;
    virtual bool setNativeFlowControl(SerialPort::FlowControl flowControl) = 0;
    virtual bool setNativeDataErrorPolicy(SerialPort::DataErrorPolicy policy) = 0;

    virtual void detectDefaultSettings() {}

    virtual bool saveOldsettings() = 0;
    virtual bool restoreOldsettings() = 0;
};

#endif // SERIALPORT_P_H
