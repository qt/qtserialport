/*
    License...
*/

#include "serialportinfo.h"
#include "serialportinfo_p.h"




/* Public methods */


QList<SerialPortInfo> SerialPortInfo::availablePorts()
{
    QList<SerialPortInfo> ports;
    // Impl me
    return ports;
}

QList<qint32> SerialPortInfo::standardRates() const
{
    QList<qint32> rates;
    // Impl me
    return rates;
}

bool SerialPortInfo::isBusy() const
{
    // Impl me
    return false;
}

bool SerialPortInfo::isValid() const
{
    // Impl me
    return false;
}
