/*
    License...
*/

#include "serialport_p.h"

#include <errno.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#if defined (Q_OS_LINUX)
  #include <linux/serial.h>
#endif

#include <QtCore/QRegExp>

/* Public */

SerialPortPrivate::SerialPortPrivate()
    : AbstractSerialPortPrivate()
    , m_descriptor(-1)
{

}

SerialPortPrivate::SerialPortPrivate(const QString &port)
    : AbstractSerialPortPrivate(port)
    , m_descriptor(-1)
{

}

bool SerialPortPrivate::open(QIODevice::OpenMode mode)
{

}

void SerialPortPrivate::close()
{
}

SerialPort::Lines SerialPortPrivate::lines() const
{

}

bool SerialPortPrivate::flush()
{

}

bool SerialPortPrivate::reset()
{

}

qint64 SerialPortPrivate::bytesAvailable() const
{

}

qint64 SerialPortPrivate::read(char *data, qint64 len)
{

}

qint64 SerialPortPrivate::write(const char *data, qint64 len)
{

}

bool SerialPortPrivate::waitForReadOrWrite(int timeout,
                                           bool checkRead, bool checkWrite,
                                           bool *selectForRead, bool *selectForWrite)
{

}

/* Protected */

static const QString defaultPathPrefix = "/dev/";

QString SerialPortPrivate::nativeToSystemLocation(const QString &port) const
{
    QString result;
    if (!port.contains(defaultPathPrefix))
        result.append(defaultPathPrefix);
    result.append(port);
    return result;
}

QString SerialPortPrivate::nativeFromSystemLocation(const QString &location) const
{
    QString result = location;
    if (result.contains(defaultPathPrefix))
        result.remove(defaultPathPrefix);
    return result;
}

bool SerialPortPrivate::setNativeRate(qint32 rate, SerialPort::Directions dir)
{

}

bool SerialPortPrivate::setNativeDataBits(SerialPort::DataBits dataBits)
{

}

bool SerialPortPrivate::setNativeParity(SerialPort::Parity parity)
{

}

bool SerialPortPrivate::setNativeStopBits(SerialPort::StopBits stopBits)
{

}

bool SerialPortPrivate::setNativeFlowControl(SerialPort::FlowControl flow)
{

}

bool SerialPortPrivate::setNativeDataInterval(int usecs)
{

}

bool SerialPortPrivate::setNativeReadTimeout(int msecs)
{

}

bool SerialPortPrivate::setNativeDataErrorPolicy(SerialPort::DataErrorPolicy policy)
{

}

void SerialPortPrivate::detectDefaultSettings()
{

}

// Used only in method SerialPortPrivate::open(SerialPort::OpenMode mode).
bool SerialPortPrivate::saveOldsettings()
{

}

// Used only in method SerialPortPrivate::close().
bool SerialPortPrivate::restoreOldsettings()
{

}

/* Private */

void SerialPortPrivate::prepareTimeouts(int msecs)
{

}

bool SerialPortPrivate::updateTermious()
{

}

bool SerialPortPrivate::setStandartRate(SerialPort::Directions dir, speed_t speed)
{

}

bool SerialPortPrivate::setCustomRate(qint32 speed)
{

}



