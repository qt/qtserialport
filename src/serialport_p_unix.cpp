/*
    License...
*/

#include "serialport_p_unix.h"


/* Public */

SerialPortPrivateUnix::SerialPortPrivateUnix()
    : SerialPortPrivate()
{
}

SerialPortPrivateUnix::~SerialPortPrivateUnix()
{
}

bool SerialPortPrivateUnix::open(QIODevice::OpenMode mode)
{

}

void SerialPortPrivateUnix::close()
{

}

bool SerialPortPrivateUnix::dtr() const
{

}

bool SerialPortPrivateUnix::rts() const
{

}

SerialPort::Lines SerialPortPrivateUnix::lines() const
{

}

bool SerialPortPrivateUnix::flush()
{

}

bool SerialPortPrivateUnix::reset()
{

}

qint64 SerialPortPrivateUnix::bytesAvailable() const
{

}

qint64 SerialPortPrivateUnix::read(char *data, qint64 len)
{

}

qint64 SerialPortPrivateUnix::write(const char *data, qint64 len)
{

}

bool SerialPortPrivateUnix::waitForReadyRead(int msec)
{

}

bool SerialPortPrivateUnix::waitForBytesWritten(int msec)
{

}

// protected

bool SerialPortPrivateUnix::setNativeRate(qint32 rate, SerialPort::Directions dir)
{

}

bool SerialPortPrivateUnix::setNativeDataBits(SerialPort::DataBits dataBits)
{

}

bool SerialPortPrivateUnix::setNativeParity(SerialPort::Parity parity)
{

}

bool SerialPortPrivateUnix::setNativeStopBits(SerialPort::StopBits stopBits)
{

}

bool SerialPortPrivateUnix::setNativeFlowControl(SerialPort::FlowControl flow)
{

}

void SerialPortPrivateUnix::setNativeDataInterval(int usecs)
{

}

void SerialPortPrivateUnix::setNativeReadTimeout(int msecs)
{

}

void SerialPortPrivateUnix::setNativeDataErrorPolicy(SerialPort::DataErrorPolicy policy)
{

}

void SerialPortPrivateUnix::detectDefaultSettings()
{

}

bool SerialPortPrivateUnix::saveOldsettings()
{

}

bool SerialPortPrivateUnix::restoreOldsettings()
{

}

/* Private */


