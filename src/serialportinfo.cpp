/*
    License...
*/

#include "serialportinfo.h"
#include "serialportinfo_p.h"
#include "serialport.h"

QT_USE_NAMESPACE

/* Public methods */


SerialPortInfo::SerialPortInfo()
    : d_ptr(new SerialPortInfoPrivate)
{
}

SerialPortInfo::SerialPortInfo(const QString &name)
    : d_ptr(new SerialPortInfoPrivate(name))
{
}

SerialPortInfo::SerialPortInfo(const SerialPortInfo &other)
    : d_ptr(other.d_ptr ? new SerialPortInfoPrivate(*other.d_ptr) : 0)
{
}

SerialPortInfo::SerialPortInfo(const SerialPort &port)
    : d_ptr(new SerialPortInfoPrivate)
{
    foreach(const SerialPortInfo &info, availablePorts()) {
        if (port.portName() == info.portName()) {
            *this = info;
            break;
        }
    }
}

SerialPortInfo::~SerialPortInfo()
{
}

void SerialPortInfo::swap(SerialPortInfo &other)
{
    d_ptr.swap(other.d_ptr);
}

SerialPortInfo& SerialPortInfo::operator=(const SerialPortInfo &other)
{
    SerialPortInfo(other).swap(*this);
    return *this;
}

QString SerialPortInfo::portName() const
{
    Q_D(const SerialPortInfo);
    return !d ? QString() : d->portName;
}

QString SerialPortInfo::systemLocation() const
{
    Q_D(const SerialPortInfo);
    return !d ? QString() : d->device;
}

QString SerialPortInfo::description() const
{
    Q_D(const SerialPortInfo);
    return !d ? QString() : d->description;
}

QString SerialPortInfo::manufacturer() const
{
    Q_D(const SerialPortInfo);
    return !d ? QString() : d->manufacturer;
}
