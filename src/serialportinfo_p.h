/*
    License...
*/

#ifndef SERIALPORTINFO_P_H
#define SERIALPORTINFO_P_H

#include "serialportinfo.h"

#include <QtCore/QString>

class SerialPortInfoPrivate
{
public:
    SerialPortInfoPrivate() {}
    SerialPortInfoPrivate(const QString &name) : portName(name) {}
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

#endif // SERIALPORTINFO_P_H
