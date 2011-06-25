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
    Q_D(SerialPort);
    d->setPort(name);
}

void SerialPort::setPort(const SerialPortInfo &info)
{
    Q_D(SerialPort);
    d->setPort(info.systemLocation());
}


QString SerialPort::portName() const
{
    Q_D(const SerialPort);
    return d->port();
}

bool SerialPort::open(OpenMode mode)
{
    Q_D(SerialPort);
    if (!isOpen()) {
        if (d->open(mode)) {
            QIODevice::open(mode);
            d->setError(SerialPort::NoError);
            return true;
        }
        else
            d->setError(SerialPort::PermissionDeniedError);

    }
    else
        d->setError(SerialPort::DeviceAlreadyOpenedError);

    return false;
}

void SerialPort::close()
{
    Q_D(SerialPort);
    if (isOpen()) {
        d->close();
        QIODevice::close();
        d->setError(SerialPort::NoError);
    }
    else
        d->setError(SerialPort::DeviceIsNotOpenedError);
}

bool SerialPort::setRate(qint32 rate, Directions dir)
{
    // Impl me
    return false;
}

qint32 SerialPort::rate(Directions dir) const
{
    Q_D(const SerialPort);
    return d->rate(dir);
}

bool SerialPort::setDataBits(DataBits dataBits)
{
    // Impl me
    return false;
}

SerialPort::DataBits SerialPort::dataBits() const
{
    Q_D(const SerialPort);
    return d->dataBits();
}

bool SerialPort::setParity(Parity parity)
{
    // Impl me
    return false;
}

SerialPort::Parity SerialPort::parity() const
{
    Q_D(const SerialPort);
    return d->parity();
}

bool SerialPort::setStopBits(StopBits stopBits)
{
    // Impl me
    return false;
}

SerialPort::StopBits SerialPort::stopBits() const
{
    Q_D(const SerialPort);
    return d->stopBits();
}

bool SerialPort::setFlowControl(FlowControl flow)
{
    // Impl me
    return false;
}

SerialPort::FlowControl SerialPort::flowControl() const
{
    Q_D(const SerialPort);
    return d->flowControl();
}

bool SerialPort::setDataInterval(int usecs)
{
    // Impl me
    return false;
}

int SerialPort::dataInterval() const
{
    Q_D(const SerialPort);
    return d->dataInterval();
}

bool SerialPort::setReadTimeout(int msecs)
{
    // Impl me
    return false;
}

int SerialPort::readTimeout() const
{
    Q_D(const SerialPort);
    return d->readTimeout();
}

bool SerialPort::dtr() const
{
    Q_D(const SerialPort);
    return d->dtr();
}

bool SerialPort::rts() const
{
    Q_D(const SerialPort);
    return d->rts();
}

SerialPort::Lines SerialPort::lines() const
{
    Q_D(const SerialPort);
    return d->lines();
}

bool SerialPort::flush()
{
    // Impl me
    return false;
}

bool SerialPort::reset()
{
    // Impl me
    return false;
}

bool SerialPort::setDataErrorPolicy(DataErrorPolicy policy)
{
    // Impl me
    return false;
}

SerialPort::DataErrorPolicy SerialPort::dataErrorPolicy() const
{
    Q_D(const SerialPort);
    return d->dataErrorPolicy();
}

SerialPort::PortError SerialPort::error() const
{
    Q_D(const SerialPort);
    return d->error();
}

void SerialPort::unsetError()
{
    Q_D(SerialPort);
    d->unsetError();
}

bool SerialPort::isSequential() const
{
    return true;
}

qint64 SerialPort::bytesAvailable() const
{
    // Impl me
    return -1;
}

qint64 SerialPort::bytesToWrite() const
{
    // Impl me
    return -1;
}

bool SerialPort::waitForReadyRead(int msecs)
{
    // Impl me
    return false;
}

bool SerialPort::waitForBytesWritten(int msecs)
{
    // Impl me
    return false;
}

bool SerialPort::setDtr(bool set)
{
    // Impl me
    return false;
}

bool SerialPort::setRts(bool set)
{
    // Impl me
    return false;
}

bool SerialPort::sendBreak(int duration)
{
    // Impl me
    return false;
}

bool SerialPort::setBreak(bool set)
{
    // Impl me
    return false;
}

qint64 SerialPort::readData(char *data, qint64 maxSize)
{
    // Impl me
    return -1;
}

qint64 SerialPort::writeData(const char *data, qint64 maxSize)
{
    // Impl me
    return -1;
}


