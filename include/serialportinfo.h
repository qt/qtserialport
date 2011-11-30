/*
    License...
*/

#ifndef SERIALPORTINFO_H
#define SERIALPORTINFO_H

#include <QtCore/qlist.h>
#include <QtCore/qscopedpointer.h>

#ifdef SERIALPORT_SHARED
#  ifdef SERIALPORT_BUILD
#    define SERIALPORT_EXPORT Q_DECL_EXPORT
#  else
#    define SERIALPORT_EXPORT Q_DECL_IMPORT
#  endif
#else
#  define SERIALPORT_EXPORT
#endif

QT_BEGIN_NAMESPACE

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
    SerialPortInfo(const QString &name);
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

    QList<qint32> standardRates() const;
    static QList<SerialPortInfo> availablePorts();

private:
    QScopedPointer<SerialPortInfoPrivate, SerialInfoPrivateDeleter> d_ptr;
};

inline bool SerialPortInfo::isNull() const
{ return !d_ptr; }

QT_END_NAMESPACE

#endif // SERIALPORTINFO_H
