/*
    License...
*/

#include "serialportinfo.h"
#include "serialportinfo_p.h"
#include "serialport.h"

static SerialPortInfoPrivate nullSerialPortInfoPrivate;

class SerialInfoPrivateDeleter
{
public:
    static inline void cleanup(SerialPortInfoPrivate *d)
    {
        if (d != &nullSerialPortInfoPrivate)
            delete d;
    }
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

SerialPortInfo::SerialPortInfo()
    : d_ptr(&nullSerialPortInfoPrivate)
{
}

SerialPortInfo::SerialPortInfo(const QString &name)
    : d_ptr(new SerialPortInfoPrivate(name))
{
    d_ptr->q_ptr = this;
}

SerialPortInfo::SerialPortInfo(const SerialPortInfo &other)
    : d_ptr(&nullSerialPortInfoPrivate)
{
    *this = other;
}

SerialPortInfo::SerialPortInfo(const SerialPort &port)
    : d_ptr(&nullSerialPortInfoPrivate)
{
    QList<SerialPortInfo> list = availablePorts();
    int size = list.size();
    for (int i = 0; i < size; ++i) {
        if (port.portName() == list[i].portName()) {
            *this = list[i];
            return;
        }
    }

    *this = SerialPortInfo();
}

SerialPortInfo::~SerialPortInfo()
{
}

SerialPortInfo& SerialPortInfo::operator=(const SerialPortInfo &other)
{
    Q_ASSERT(d_ptr);
    d_ptr.reset(new SerialPortInfoPrivate(*other.d_ptr));
    d_ptr->q_ptr = this;
    return *this;
}

QString SerialPortInfo::portName() const
{
    Q_D(const SerialPortInfo);
    return d->m_portName;
}

QString SerialPortInfo::systemLocation() const
{
    Q_D(const SerialPortInfo);
    return d->m_device;
}

QString SerialPortInfo::description() const
{
    Q_D(const SerialPortInfo);
    return d->m_description;
}

QString SerialPortInfo::manufacturer() const
{
    Q_D(const SerialPortInfo);
    return d->m_manufacturer;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

SerialPortInfoPrivate::SerialPortInfoPrivate()
    : q_ptr(0)
{
}

SerialPortInfoPrivate::SerialPortInfoPrivate(const QString &name)
    : m_portName(name)
    , q_ptr(0)
{
}

SerialPortInfoPrivate::~SerialPortInfoPrivate()
{
}

