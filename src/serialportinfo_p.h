/*
    License...
*/

#ifndef SERIALPORTINFO_P_H
#define SERIALPORTINFO_P_H

#include "serialportinfo.h"

#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

class SerialPortInfoPrivate
{
public:
    SerialPortInfoPrivate() {}
    ~SerialPortInfoPrivate() {}

    QString portName;
    QString device;
    QString description;
    QString manufacturer;
};

class SerialInfoPrivateDeleter
{
public:
    static void cleanup(SerialPortInfoPrivate *p) {
        delete p;
    }
};

QT_END_NAMESPACE

#endif // SERIALPORTINFO_P_H
