/*
    License...
*/

#ifndef SERIALPORTENGINE_SYMBIAN_P_H
#define SERIALPORTENGINE_SYMBIAN_P_H

#include "serialport.h"
#include "serialportengine_p.h"

#include <c32comm.h>

QT_BEGIN_NAMESPACE_SERIALPORT

class SymbianSerialPortEngine : public QObject, public SerialPortEngine
{
    Q_OBJECT
public:
    SymbianSerialPortEngine(SerialPortPrivate *d);
    virtual ~SymbianSerialPortEngine();

    virtual bool open(const QString &location, QIODevice::OpenMode mode);
    virtual void close(const QString &location);

    virtual SerialPort::Lines lines() const;

    virtual bool setDtr(bool set);
    virtual bool setRts(bool set);

    virtual bool flush();
    virtual bool reset();

    virtual bool sendBreak(int duration);
    virtual bool setBreak(bool set);

    virtual qint64 bytesAvailable() const;
    virtual qint64 bytesToWrite() const;

    virtual qint64 read(char *data, qint64 len);
    virtual qint64 write(const char *data, qint64 len);
    virtual bool select(int timeout,
                        bool checkRead, bool checkWrite,
                        bool *selectForRead, bool *selectForWrite);

    virtual QString toSystemLocation(const QString &port) const;
    virtual QString fromSystemLocation(const QString &location) const;

    virtual bool setRate(qint32 rate, SerialPort::Directions dir);
    virtual bool setDataBits(SerialPort::DataBits dataBits);
    virtual bool setParity(SerialPort::Parity parity);
    virtual bool setStopBits(SerialPort::StopBits stopBits);
    virtual bool setFlowControl(SerialPort::FlowControl flowControl);

    virtual bool setDataErrorPolicy(SerialPort::DataErrorPolicy policy);

    virtual bool isReadNotificationEnabled() const;
    virtual void setReadNotificationEnabled(bool enable);
    virtual bool isWriteNotificationEnabled() const;
    virtual void setWriteNotificationEnabled(bool enable);

    virtual bool processIOErrors();

public:
    static qint32 rateFromSetting(EBps setting);
    static EBps settingFromRate(qint32 rate);
    static QList<qint32> standardRates();

protected:
    virtual void detectDefaultSettings();
    //virtual bool eventFilter(QObject *obj, QEvent *e);

private:
    bool updateCommConfig();

private:
    TCommConfig m_currentSettings;
    TCommConfig m_restoredSettings;
    RComm m_descriptor;
    mutable RTimer m_selectTimer;
};

QT_END_NAMESPACE_SERIALPORT

#endif // SERIALPORTENGINE_SYMBIAN_P_H
