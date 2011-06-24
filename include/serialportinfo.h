/*
    License...
*/

#ifndef SERIALPORTINFO_H
#define SERIALPORTINFO_H

#include <QtCore/QList>
#include <QtCore/QScopedPointer>

#ifdef SERIALPORT_SHARED
#  ifdef SERIALPORT_BUILD
#    define SERIALPORT_EXPORT Q_DECL_EXPORT
#  else
#    define SERIALPORT_EXPORT Q_DECL_IMPORT
#  endif
#else
#  define SERIALPORT_EXPORT
#endif

class SerialPort;
class SerialPortInfoPrivate;
class SerialInfoPrivateDeleter;

class SERIALPORT_EXPORT SerialPortInfo
{
    Q_DECLARE_PRIVATE(SerialPortInfo)
public:
    SerialPortInfo();
    SerialPortInfo(const SerialPortInfo &other);
    SerialPortInfo(const SerialPort &port);
    ~SerialPortInfo();

    SerialPortInfo& operator=(const SerialPortInfo &other);
    void swap(SerialPortInfo &other);

    QString portName() const;
    QString systemLocation() const;
    QString description() const;
    QString manufacturer() const;

    bool isNull() const;
    bool isBusy() const;
    bool isValid() const;

    QList<int> standardRates() const;
    static QList<SerialPortInfo> availablePorts();

private:
    SerialPortInfo(const QString &name);
    QScopedPointer<SerialPortInfoPrivate, SerialInfoPrivateDeleter> d_ptr;
};

inline bool SerialPortInfo::isNull() const
{ return !d_ptr; }

#endif // SERIALPORTINFO_H
