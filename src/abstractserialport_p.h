/*
    License...
*/

#ifndef ABSTRACTSERIALPORT_P_H
#define ABSTRACTSERIALPORT_P_H

#include "serialport.h"

class AbstractSerialPortPrivate
{
public:
    AbstractSerialPortPrivate()
        : m_inRate(SerialPort::UnknownRate)
        , m_outRate(SerialPort::UnknownRate)
        , m_dataBits(SerialPort::UnknownDataBits)
        , m_parity(SerialPort::UnknownParity)
        , m_stopBits(SerialPort::UnknownStopBits)
        , m_flow(SerialPort::UnknownFlowControl)
        , m_dataInterval(0)
        , m_readTimeout(0)
        , m_policy(SerialPort::IgnorePolicy)
        , m_portError(SerialPort::NoError)
        , m_oldSettingsIsSaved(false)
    {}

    void setPort(const QString &port) { m_systemLocation = nativeToSystemLocation(port); }
    QString port() const { return nativeFromSystemLocation(m_systemLocation); }

    virtual bool open(QIODevice::OpenMode mode) = 0;
    virtual void close() = 0;

    bool setRate(qint32 rate, SerialPort::Directions dir) {
        if (setNativeRate(rate, dir)) {
            if (SerialPort::Input & dir)
                m_inRate = rate;
            if (SerialPort::Output & dir)
                m_outRate = rate;
            return true;
        }
        return false;
    }

    qint32 rate(SerialPort::Directions dir) const {
        if (SerialPort::AllDirections == dir)
            return (m_inRate == m_outRate) ? (m_inRate) : SerialPort::UnknownRate;
        return (SerialPort::Input & dir) ? (m_inRate) : (m_outRate);
    }

    bool setDataBits(SerialPort::DataBits dataBits) {
        if (setNativeDataBits(dataBits)) {
            m_dataBits = dataBits;
            return true;
        }
        return false;
    }

    SerialPort::DataBits dataBits() const { return m_dataBits; }

    bool setParity(SerialPort::Parity parity) {
        if (setNativeParity(parity)) {
            m_parity = parity;
            return true;
        }
        return false;
    }

    SerialPort::Parity parity() const { return m_parity; }

    bool setStopBits(SerialPort::StopBits stopBits) {
        if (setNativeStopBits(stopBits)) {
            m_stopBits = stopBits;
            return true;
        }
        return false;
    }

    SerialPort::StopBits stopBits() const { return m_stopBits; }

    bool setFlowControl(SerialPort::FlowControl flow) {
        if (setNativeFlowControl(flow)) {
            m_flow = flow;
            return true;
        }
        return false;
    }

    SerialPort::FlowControl flowControl() const { return m_flow; }

    bool setDataInterval(int usecs) {
        if (setNativeDataInterval(usecs)) {
            m_dataInterval = usecs;
            return true;
        }
        return false;
    }

    int dataInterval() const { return m_dataInterval; }

    bool setReadTimeout(int msecs) {
        if (setNativeReadTimeout(msecs)) {
            m_readTimeout = msecs;
            return true;
        }
        return false;
    }

    int readTimeout() const { return m_readTimeout; }

    bool dtr() const { return SerialPort::Dtr & lines(); }
    bool rts() const { return SerialPort::Rts & lines(); }

    virtual SerialPort::Lines lines() const = 0;

    virtual bool flush() = 0;
    virtual bool reset() = 0;

    bool setDataErrorPolicy(SerialPort::DataErrorPolicy policy) {
        if (setNativeDataErrorPolicy(policy)) {
            m_policy = policy;
            return true;
        }
        return false;
    }

    SerialPort::DataErrorPolicy dataErrorPolicy() const { return m_policy; }

    SerialPort::PortError error() const { return m_portError; }
    void unsetError() { m_portError = SerialPort::NoError; }

    void setError(SerialPort::PortError error) { m_portError = error; }

    virtual qint64 bytesAvailable() const = 0;

    virtual qint64 read(char *data, qint64 len) = 0;
    virtual qint64 write(const char *data, qint64 len) = 0;
    virtual bool waitForReadOrWrite(int timeout,
                                    bool checkRead, bool checkWrite,
                                    bool *selectForRead, bool *selectForWrite) = 0;

protected:
    //General (for any OS) private parameters
    QString m_systemLocation;
    qint32 m_inRate;
    qint32 m_outRate;
    SerialPort::DataBits m_dataBits;
    SerialPort::Parity m_parity;
    SerialPort::StopBits m_stopBits;
    SerialPort::FlowControl m_flow;
    int m_dataInterval;
    int m_readTimeout;
    SerialPort::DataErrorPolicy m_policy;
    SerialPort::PortError m_portError;

    bool m_oldSettingsIsSaved;

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

    virtual void detectDefaultSettings() = 0;

    virtual bool saveOldsettings() = 0;
    virtual bool restoreOldsettings() = 0;
};

#endif // ABSTRACTSERIALPORT_P_H
