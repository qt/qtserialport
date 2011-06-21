/*
    License...
*/

#include "serialport.h"
#include "serialportinfo.h"

#include "serialport_p.h"


SerialPort::SerialPort(QObject *parent)
    : QIODevice(parent)
    , d_ptr(new SerialPortPrivate())
{

}


SerialPort::SerialPort(const QString &name, QObject *parent)
    : QIODevice(parent)
    , d_ptr(new SerialPortPrivate())
{

}

SerialPort::SerialPort(const SerialPortInfo &info, QObject *parent)
    : QIODevice(parent)
    , d_ptr(new SerialPortPrivate())
{

}

SerialPort::~SerialPort()
{
    /**/
    close();
    delete d_ptr;
}

void SerialPort::setPort(const QString &name)
{

}

void SerialPort::setPort(const SerialPortInfo &info)
{

}


QString SerialPort::portName() const
{

}

bool SerialPort::open(OpenMode mode)
{

}

void SerialPort::close()
{

}

bool SerialPort::setRate(qint32 rate, Directions dir)
{

}

qint32 SerialPort::rate(Directions dir) const
{

}

bool SerialPort::setDataBits(DataBits dataBits)
{

}

SerialPort::DataBits SerialPort::dataBits() const
{

}

bool SerialPort::setParity(Parity parity)
{

}

SerialPort::Parity SerialPort::parity() const
{

}

bool SerialPort::setStopBits(StopBits stopBits)
{

}

SerialPort::StopBits SerialPort::stopBits() const
{

}

bool SerialPort::setFlowControl(FlowControl flow)
{

}

SerialPort::FlowControl SerialPort::flowControl() const
{

}

void SerialPort::setDataInterval(int usecs)
{

}

int SerialPort::dataInterval() const
{

}

void SerialPort::setReadTimeout(int msecs)
{

}

int SerialPort::readTimeout() const
{

}

bool SerialPort::dtr() const
{

}

bool SerialPort::rts() const
{

}

SerialPort::Lines SerialPort::lines() const
{

}

bool SerialPort::flush()
{

}

bool SerialPort::reset()
{

}

void SerialPort::setDataErrorPolicy(DataErrorPolicy policy)
{

}

SerialPort::DataErrorPolicy SerialPort::dataErrorPolicy() const
{

}

SerialPort::PortError SerialPort::error() const
{

}

void SerialPort::unsetError()
{

}

bool SerialPort::isSequential() const
{

}

qint64 SerialPort::bytesAvailable() const
{

}

qint64 SerialPort::bytesToWrite() const
{

}

bool SerialPort::waitForReadyRead(int msecs)
{

}

bool SerialPort::waitForBytesWritten(int msecs)
{

}

bool SerialPort::setDtr(bool set)
{

}

bool SerialPort::setRts(bool set)
{

}

bool SerialPort::sendBreak(int duration)
{

}

bool SerialPort::setBreak(bool set)
{

}

qint64 SerialPort::readData(char *data, qint64 maxSize)
{

}

qint64 SerialPort::writeData(const char *data, qint64 maxSize)
{

}


