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

    SerialPortPrivate(const QString &port)
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
    {

    }

    void setPort(const QString &port) { m_systemLocation = nativeToSystemLocation(port); }
    QString port() const { return nativeFromSystemLocation(m_systemLocation); }

    virtual bool open(QIODevice::OpenMode mode) {}
    virtual void close() {}

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

    virtual bool dtr() const {}
    virtual bool rts() const {}

    virtual SerialPort::Lines lines() const {}

    virtual bool flush() {}
    virtual bool reset() {}

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

    virtual qint64 bytesAvailable() const {}

    virtual qint64 read(char *data, qint64 len) {}
    virtual qint64 write(const char *data, qint64 len) {}
    virtual bool waitForReadOrWrite(int timeout,
                             bool checkRead, bool checkWrite,
                             bool *selectForRead, bool *selectForWrite) {}

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

    virtual QString nativeToSystemLocation(const QString &port) const {}
    virtual QString nativeFromSystemLocation(const QString &location) const {}

    virtual bool setNativeRate(qint32 rate, SerialPort::Directions dir) {}
    virtual bool setNativeDataBits(SerialPort::DataBits dataBits) {}
    virtual bool setNativeParity(SerialPort::Parity parity) {}
    virtual bool setNativeStopBits(SerialPort::StopBits stopBits) {}
    virtual bool setNativeFlowControl(SerialPort::FlowControl flowControl) {}

    virtual bool setNativeDataInterval(int usecs) {}
    virtual bool setNativeReadTimeout(int msecs) {}

    virtual bool setNativeDataErrorPolicy(SerialPort::DataErrorPolicy policy) {}

    virtual void detectDefaultSettings() {}

    virtual bool saveOldsettings() {}
    virtual bool restoreOldsettings() {}
};

#endif // SERIALPORT_P_H
