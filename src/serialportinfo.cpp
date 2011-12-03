/*
    License...
*/

#include "serialportinfo.h"
#include "serialportinfo_p.h"
#include "serialport.h"

QT_USE_NAMESPACE


/*!
    \class SerialPortInfo
    \brief The SerialPortInfo class gives access to information about
    existing serial ports.

    \ingroup serial
    \inmodule QSerialDevice

    Use the static functions to generate a list of SerialPortInfo objects.
    Each SerialPortInfo object in the list represents a single serial port
    and can be queried for port name, system location, description,
    manufacturer. SerialPortInfo also be used as an input parameter to
    the method setPort() a class SerialPort.

    Algorithm for obtaining information on the serial ports is platform
    specific and very different for individual platforms. The more so that
    the information about same device (serial port) obtained on the current
    platform may not correspond to the information received on a different
    platform. For example, names of ports and their systemic location, of
    course, different for different platforms. Also such parameters as a
    string description and manufacturer may vary.

    So, the details of the semantics of information and its sources
    for a variety of platforms is presented below.

    \section1 Port name

    Is the name of the device in a shorter form, which is usually represented
    in the interface of the OS for human beings.

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
    Constructs a SerialPortInfo object from serial port \a name.

    ...
*/
SerialPortInfo::SerialPortInfo(const QString &name)
    : d_ptr(new SerialPortInfoPrivate)
{
    foreach(const SerialPortInfo &info, availablePorts()) {
        if (name == info.portName()) {
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

/*!
    Bla bla \a other
*/
void SerialPortInfo::swap(SerialPortInfo &other)
{
    d_ptr.swap(other.d_ptr);
}

/*!
    Bla bla \a other
*/
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
    \fn QList<qint32> SerialPortInfo::standardRates() const

    Returns a list of available standard baud rates supported by
    the current serial port.
*/

/*!
    \fn QList<SerialPortInfo> SerialPortInfo::availablePorts()

    Returns a list of available serial ports on the system.
*/
