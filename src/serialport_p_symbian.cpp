/*
    License...
*/

#include "serialport_p.h"

#include <QtCore/QRegExp>
//#include <QtCore/QDebug>



/* Public methods */


SerialPortPrivate::SerialPortPrivate()
    : AbstractSerialPortPrivate()
{
    // Impl me

    m_notifier.setRef(this);
}

bool SerialPortPrivate::open(QIODevice::OpenMode mode)
{
    // Impl me
    setError(SerialPort::ConfiguringError);
    return false;
}

void SerialPortPrivate::close()
{
    // Impl me
}

SerialPort::Lines SerialPortPrivate::lines() const
{
    SerialPort::Lines ret = 0;
    // Impl me
    return ret;
}

bool SerialPortPrivate::setDtr(bool set)
{
    // Impl me
    return false;
}

bool SerialPortPrivate::setRts(bool set)
{
    // Impl me
    return false;
}

bool SerialPortPrivate::reset()
{
    // Impl me
    return false;
}

bool SerialPortPrivate::sendBreak(int duration)
{
    // Impl me
    return false;
}

bool SerialPortPrivate::setBreak(bool set)
{
    // Impl me
    return false;
}

qint64 SerialPortPrivate::bytesAvailable() const
{
    // Impl me
    return -1;
}

qint64 SerialPortPrivate::bytesToWrite() const
{
    // Impl me
    return -1;
}

qint64 SerialPortPrivate::read(char *data, qint64 len)
{
    // Impl me
    return -1;
}

qint64 SerialPortPrivate::write(const char *data, qint64 len)
{
    // Impl me
    return -1;
}

bool SerialPortPrivate::waitForReadOrWrite(int timeout,
                                           bool checkRead, bool checkWrite,
                                           bool *selectForRead, bool *selectForWrite)
{
    // Impl me
    return false;
}


/* Protected methods */


static const QString defaultPathPostfix = ":";

QString SerialPortPrivate::nativeToSystemLocation(const QString &port) const
{
    // Port name is equval to port location.
    return port;
}

QString SerialPortPrivate::nativeFromSystemLocation(const QString &location) const
{
    // Port name is equval to port location.
    return location;
}

bool SerialPortPrivate::setNativeRate(qint32 rate, SerialPort::Directions dir)
{
    if ((rate == SerialPort::UnknownRate) || (dir != SerialPort::AllDirections)) {
        setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }

    bool ret = false;
    // Impl me

    if (!ret)
        setError(SerialPort::ConfiguringError);
    return ret;
}

bool SerialPortPrivate::setNativeDataBits(SerialPort::DataBits dataBits)
{
    if ((dataBits == SerialPort::UnknownDataBits)
            || isRestrictedAreaSettings(dataBits, m_stopBits)) {

        setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }

    bool ret = false;
    // Impl me

    if (!ret)
        setError(SerialPort::ConfiguringError);
    return ret;
}

bool SerialPortPrivate::setNativeParity(SerialPort::Parity parity)
{
    if (parity == SerialPort::UnknownParity) {
        setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }

    bool ret = false;
    // Impl me

    if (!ret)
        setError(SerialPort::ConfiguringError);
    return ret;
}

bool SerialPortPrivate::setNativeStopBits(SerialPort::StopBits stopBits)
{
    if ((stopBits == SerialPort::UnknownStopBits)
            || isRestrictedAreaSettings(m_dataBits, stopBits)) {

        setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }

    bool ret = false;
    // Impl me

    if (!ret)
        setError(SerialPort::ConfiguringError);
    return ret;
}

bool SerialPortPrivate::setNativeFlowControl(SerialPort::FlowControl flow)
{
    if (flow == SerialPort::UnknownFlowControl) {
        setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }

    bool ret = false;
    // Impl me

    if (!ret)
        setError(SerialPort::ConfiguringError);
    return ret;
}

bool SerialPortPrivate::setNativeDataInterval(int usecs)
{
    Q_UNUSED(usecs)
    // Impl me
    return false;
}

bool SerialPortPrivate::setNativeReadTimeout(int msecs)
{
    // Impl me
    return false;
}

bool SerialPortPrivate::setNativeDataErrorPolicy(SerialPort::DataErrorPolicy policy)
{
    Q_UNUSED(policy)
    return true;
}

bool SerialPortPrivate::nativeFlush()
{
    // Impl me
    return false;
}

void SerialPortPrivate::detectDefaultSettings()
{
    // Impl me
}

// Used only in method SerialPortPrivate::open().
bool SerialPortPrivate::saveOldsettings()
{
    // Impl me
    return false;
}

// Used only in method SerialPortPrivate::close().
bool SerialPortPrivate::restoreOldsettings()
{
    // Impl me
    return false;
}


/* Private methods */



void SerialPortPrivate::recalcTotalReadTimeoutConstant()
{

}

bool SerialPortPrivate::isRestrictedAreaSettings(SerialPort::DataBits dataBits,
                                                 SerialPort::StopBits stopBits) const
{
    // Impl me
    return (((dataBits == SerialPort::Data5) && (stopBits == SerialPort::TwoStop))
            || ((dataBits == SerialPort::Data6) && (stopBits == SerialPort::OneAndHalfStop))
            || ((dataBits == SerialPort::Data7) && (stopBits == SerialPort::OneAndHalfStop))
            || ((dataBits == SerialPort::Data8) && (stopBits == SerialPort::OneAndHalfStop)));
}

// Prepares other parameters of the structures port configuration.
// Used only in method SerialPortPrivate::open().
void SerialPortPrivate::prepareOtherOptions()
{
    // Impl me
}



