/*
*/

#ifndef SERIALPORTINFO_P_H
#define SERIALPORTINFO_P_H

#include <QtCore/QString>
#include "serialportinfo.h"


class SerialPortInfoPrivate
{
public:
    SerialPortInfoPrivate(const QString &port);

    QString portName;
    QString device;
    QString description;
    QString manufacturer;

    bool isBusy() const;
};

#endif // SERIALPORTINFO_P_H
