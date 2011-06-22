/*
    License...
*/

#include "serialportinfo.h"
#include "serialport.h"

#include "serialportinfo_p.h"


SerialPortInfo::SerialPortInfo()
    : d_ptr(0)
{}

SerialPortInfo::SerialPortInfo(const QString &port)
    : d_ptr(new SerialPortInfoPrivate(port))
{}

SerialPortInfo::SerialPortInfo(const SerialPortInfo &other)
    : d_ptr(other.d_ptr ? new SerialPortInfoPrivate(*other.d_ptr) : 0)
{}

SerialPortInfo::SerialPortInfo(const SerialPort &port)
    : d_ptr(new SerialPortInfoPrivate(port.portName()))
{}

SerialPortInfo::~SerialPortInfo()
{
    delete d_ptr;
}

SerialPortInfo& SerialPortInfo::operator=(const SerialPortInfo &other)
{
    SerialPortInfo(other).swap(*this);
    return *this;
}

void SerialPortInfo::swap(SerialPortInfo &other)
{
    qSwap(other.d_ptr, d_ptr);
}

QString SerialPortInfo::portName() const
{
    Q_D(const SerialPortInfo);
    return d ? d->portName : QString();
}

QString SerialPortInfo::systemLocation() const
{
    Q_D(const SerialPortInfo);
    return d ? d->device : QString();
}

QString SerialPortInfo::description() const
{
    Q_D(const SerialPortInfo);
    return d ? d->description : QString();
}

QString SerialPortInfo::manufacturer() const
{
    Q_D(const SerialPortInfo);
    return d ? d->manufacturer : QString();
}

bool SerialPortInfo::isNull() const
{
    return (d_ptr == 0);
}

bool SerialPortInfo::isBusy() const
{
    Q_D(const SerialPortInfo);
    return d ? d->isBusy() : false;
}

bool SerialPortInfo::isValid() const
{
    Q_D(const SerialPortInfo);
    /*
    if (d != 0)
        return SerialPort(d->portName).open();
    return false;
    */
}

QList<int> SerialPortInfo::standardRates() const
{
    QList<int> ret;
    Q_D(const SerialPortInfo);
    if (d != 0) {
        //
    }
    return ret;
}

QList<SerialPortInfo> SerialPortInfo::availablePorts()
{

}
