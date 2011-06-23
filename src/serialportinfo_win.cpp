/*
    License...
*/

#include "serialportinfo.h"
#include "serialportinfo_p.h"

#include <qt_windows.h>




QList<SerialPortInfo> SerialPortInfo::availablePorts()
{
    QList<SerialPortInfo> ports;

    /*
      Windows implementation.
      ...
      ...
    */

    // for (....) {

    SerialPortInfo info;
    info.d_ptr->m_portName = "blabla";
    info.d_ptr->m_device = "blabla";
    info.d_ptr->m_description = "blabla";
    info.d_ptr->m_manufacturer = "blabla";

    // } //end for

    return ports;
}


QList<int> SerialPortInfo::standardRates() const
{
    QList<int> rates;

    /*
      Windows implementation detect supported rates list
      or append standart Windows rates.
    */

    return rates;
}


bool SerialPortInfo::isBusy() const
{
    QString location = systemLocation();
    QByteArray nativeFilePath = QByteArray((const char *)location.utf16(), location.size() * 2 + 1);

    // Try open and close port by location: nativeFilePath
    //...
    //...

    return false;
}

bool SerialPortInfo::isValid() const
{

}
