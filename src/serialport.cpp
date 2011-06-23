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
}

qint32 SerialPort::rate(Directions dir) const
{
    Q_D(const SerialPort);
    return d->rate(dir);
}

bool SerialPort::setDataBits(DataBits dataBits)
{

}

SerialPort::DataBits SerialPort::dataBits() const
{
    Q_D(const SerialPort);
    return d->dataBits();
}

bool SerialPort::setParity(Parity parity)
{

}

SerialPort::Parity SerialPort::parity() const
{
    Q_D(const SerialPort);
    return d->parity();
}

bool SerialPort::setStopBits(StopBits stopBits)
{

}

SerialPort::StopBits SerialPort::stopBits() const
{
    Q_D(const SerialPort);
    return d->stopBits();
}

bool SerialPort::setFlowControl(FlowControl flow)
{

}

SerialPort::FlowControl SerialPort::flowControl() const
{
    Q_D(const SerialPort);
    return d->flowControl();
}

bool SerialPort::setDataInterval(int usecs)
{

}

int SerialPort::dataInterval() const
{
    Q_D(const SerialPort);
    return d->dataInterval();
}

bool SerialPort::setReadTimeout(int msecs)
{

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

}

bool SerialPort::reset()
{

}

bool SerialPort::setDataErrorPolicy(DataErrorPolicy policy)
{

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


