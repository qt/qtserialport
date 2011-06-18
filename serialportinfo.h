/*
    License...
*/
 
#ifndef SERIALPORTINFO_H
#define SERIALPORTINFO_H
 
class SerialPort;
class SerialPortInfoPrivate;
 
class QSERIALPORT_EXPORT SerialPortInfo
{
public:
   SerialPortInfo(); //produces NULL info
   SerialPortInfo(const SerialPortInfo &other); //copy constructor
   explicit SerialPortInfo(const SerialPort &port);
   ~SerialPortInfo();
 
   SerialPortInfo& operator=(const SerialPortInfo &other);
   void swap(SerialPortInfo &other);
 
   QString portName() const;       // << Names, e.g.: "COM1", "ttyS0". 
   QString systemLocation() const; // << System path, e.g.: "\\\\\\\\.\\\\COM24", "/dev/ttyS45". 
   QString description() const;    // << Description, e.g.: "Serial port", "Prolific USB-to-Serial Comm Port", "Motorola Phone (E1 iTunes)".
   QString manufacturer() const;   // << Manufacturer, e.g.: "(Standarts ports)", "Prolific Technology Inc.". 
 
   bool isNull() const;
   bool isBusy() const;
   bool isValid() const;          // << If the port is actually exists.
   
   QList<int> standardRates() const;
   static QList<SerialPortInfo> availablePorts();
   
private:
    Q_DECLARE_PRIVATE(SerialPortInfo)
    SerialPortInfoPrivate * d_ptr;
};
 
#endif // SERIALPORTINFO_H