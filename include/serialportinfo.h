/*
    License...
*/
 
#ifndef SERIALPORTINFO_H
# define SERIALPORTINFO_H

# include <QtCore/QList>

# ifdef SERIALPORT_SHARED
#  ifdef SERIALPORT_BUILD
#   define SERIALPORT_EXPORT Q_DECL_EXPORT
#  else
#   define SERIALPORT_EXPORT Q_DECL_IMPORT
#  endif
# else
#  define SERIALPORT_EXPORT
# endif

class SerialPort;
class SerialPortInfoPrivate;
 
class SERIALPORT_EXPORT SerialPortInfo
{
public:
    SerialPortInfo();
    SerialPortInfo(const QString &port);
    SerialPortInfo(const SerialPortInfo &other);
    explicit SerialPortInfo(const SerialPort &port);
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
    Q_DECLARE_PRIVATE(SerialPortInfo)
    SerialPortInfoPrivate *d_ptr;
};
 
#endif // SERIALPORTINFO_H
