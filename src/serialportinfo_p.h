/*
    License...
*/

#ifndef SERIALPORTINFO_P_H
#define SERIALPORTINFO_P_H

#include "serialportinfo.h"

#include <QtCore/QString>

struct SerialPortInfoPrivate
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

struct SerialInfoPrivateDeleter
{
    static void cleanup(SerialPortInfoPrivate *p) {
        delete p;
    }
};

#endif // SERIALPORTINFO_P_H
