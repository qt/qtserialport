/*
    License...
*/

#ifndef SERIALPORTINFO_P_H
#define SERIALPORTINFO_P_H

#include "serialportinfo.h"

#include <QtCore/QString>

class SerialPortInfoPrivate
{
    Q_DECLARE_PUBLIC(SerialPortInfo)
public:
    SerialPortInfoPrivate();
    SerialPortInfoPrivate(const QString &name);
    ~SerialPortInfoPrivate();

private:
    QString m_portName;
    QString m_device;
    QString m_description;
    QString m_manufacturer;

    SerialPortInfo *q_ptr;
};

#endif // SERIALPORTINFO_P_H
