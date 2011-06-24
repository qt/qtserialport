/*
*/

#ifndef ABSTRACTSERIALPORTINFO_P_H
#define ABSTRACTSERIALPORTINFO_P_H

#include <QtCore/QString>

class AbstractSerialPortInfoPrivate
{
public:
    QString portName;
    QString device;
    QString description;
    QString manufacturer;

    virtual bool isBusy() const = 0;
    virtual bool isValid() const = 0;
};

#endif // SERIALPORTINFO_P_H
