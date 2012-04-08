/*
    License...
*/

#ifndef SERIALPORTINFO_H
#define SERIALPORTINFO_H

#include <QtCore/qlist.h>
#include <QtCore/qscopedpointer.h>

#include "serialport-global.h"

QT_BEGIN_NAMESPACE_SERIALPORT

class SerialPort;
class SerialPortInfoPrivate;
class SerialInfoPrivateDeleter;

class Q_ADDON_SERIALPORT_EXPORT SerialPortInfo
{
    Q_DECLARE_PRIVATE(SerialPortInfo)
public:
    SerialPortInfo();
    SerialPortInfo(const SerialPortInfo &other);
    SerialPortInfo(const SerialPort &port);
    SerialPortInfo(const QString &name);
    ~SerialPortInfo();

    SerialPortInfo& operator=(const SerialPortInfo &other);
    void swap(SerialPortInfo &other);

    QString portName() const;
    QString systemLocation() const;
    QString description() const;
    QString manufacturer() const;
    QString vendorIdentifier() const;
    QString productIdentifier() const;

    bool isNull() const;
    bool isBusy() const;
    bool isValid() const;

    static QList<qint32> standardRates();
    static QList<SerialPortInfo> availablePorts();

private:
    QScopedPointer<SerialPortInfoPrivate, SerialInfoPrivateDeleter> d_ptr;
};

inline bool SerialPortInfo::isNull() const
{ return !d_ptr; }

QT_END_NAMESPACE_SERIALPORT

#endif // SERIALPORTINFO_H
