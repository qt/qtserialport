/*
    License...
*/

#ifndef SERIALPORTINFO_P_H
#define SERIALPORTINFO_P_H

#include "serialportinfo.h"

#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE_SERIALPORT

class SerialPortInfoPrivate
{
public:
    SerialPortInfoPrivate() {}
    ~SerialPortInfoPrivate() {}

    QString portName;
    QString device;
    QString description;
    QString manufacturer;
    QString vendorIdentifier;
    QString productIdentifier;
};

class SerialInfoPrivateDeleter
{
public:
    static void cleanup(SerialPortInfoPrivate *p) {
        delete p;
    }
};

QT_END_NAMESPACE_SERIALPORT

#endif // SERIALPORTINFO_P_H
