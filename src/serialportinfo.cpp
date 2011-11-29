/*
    License...
*/

#include "serialportinfo.h"
#include "serialportinfo_p.h"
#include "serialport.h"

QT_USE_NAMESPACE


/*!
    \class SerialPortInfo
    \brief The SerialPortInfo class gives access to information about ...

    \ingroup network??
    \inmodule QtNetwork??

    SerialPortInfo ... (full description)
    ...
    ...


    \sa SerialPort
*/

/* Public methods */

/*!
    Constructs an empty SerialPortInfo object.

    \sa isNull()
*/
SerialPortInfo::SerialPortInfo()
    : d_ptr(new SerialPortInfoPrivate)
{
}

/*!
    Constructs a SerialPortInfo object from serial port \a name.

    ...
*/
SerialPortInfo::SerialPortInfo(const QString &name)
    : d_ptr(new SerialPortInfoPrivate(name))
{
}

/*!
    Constructs a copy of \a other.
*/
SerialPortInfo::SerialPortInfo(const SerialPortInfo &other)
    : d_ptr(other.d_ptr ? new SerialPortInfoPrivate(*other.d_ptr) : 0)
{
}

/*!
    Constructs a SerialPortInfo object from serial \a port.
*/
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

/*!
    Destroys the SerialPortInfo object. References to the values in the
    object become invalid.
*/
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

/*!
    Returns the name of the serial port.
*/
QString SerialPortInfo::portName() const
{
    Q_D(const SerialPortInfo);
    return !d ? QString() : d->portName;
}

/*!
    Returns the system location of the serial port.
*/
QString SerialPortInfo::systemLocation() const
{
    Q_D(const SerialPortInfo);
    return !d ? QString() : d->device;
}

/*!
    Returns the description of the serial port,
    if available.
*/
QString SerialPortInfo::description() const
{
    Q_D(const SerialPortInfo);
    return !d ? QString() : d->description;
}

/*!
    Returns the manufacturer of the serial port,
    if available.
*/
QString SerialPortInfo::manufacturer() const
{
    Q_D(const SerialPortInfo);
    return !d ? QString() : d->manufacturer;
}

/*!
    \fn bool SerialPortInfo::isNull() const

    Returns whether this SerialPortInfo object holds a
    serial port definition.
*/

/*!
    \fn bool SerialPortInfo::isBusy() const

    Returns true if serial port is busy;
    otherwise false.
*/

/*!
    \fn bool SerialPortInfo::isValid() const

    Returns true if serial port is present on system;
    otherwise false.
*/

/*!
    \fn QList<qint32> SerialPortInfo::standardRates()

    Returns a list of available standard baud rates supported by
    the current serial port.
*/

/*!
    \fn QList<SerialPortInfo> QPrinterInfo::availablePorts()

    Returns a list of available serial ports on the system.
*/
