/*
    License...
*/

#ifndef SERIALPORTENGINE_P_UNIX_H
#define SERIALPORTENGINE_P_UNIX_H

#include "serialport.h"
#include "serialportengine_p.h"

#include <termios.h>
//#  undef CMSPAR

QT_BEGIN_NAMESPACE

class QSocketNotifier;

class UnixSerialPortEngine : public QObject, public SerialPortEngine
{
    Q_OBJECT
public:
    UnixSerialPortEngine(SerialPortPrivate *parent);
    virtual ~UnixSerialPortEngine();

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

protected:
    virtual void detectDefaultSettings();
    virtual bool eventFilter(QObject *obj, QEvent *e);

private:
    struct termios m_currTermios;
    struct termios m_oldTermios;
    int m_descriptor;

    QSocketNotifier *m_readNotifier;
    QSocketNotifier *m_writeNotifier;
    QSocketNotifier *m_exceptionNotifier;

    bool updateTermios();
    bool setStandartRate(SerialPort::Directions dir, speed_t rate);
    bool setCustomRate(qint32 rate);

#if !defined (CMSPAR)
    qint64 writePerChar(const char *data, qint64 maxSize);
#endif
    qint64 readPerChar(char *data, qint64 maxSize);
};

QT_END_NAMESPACE

#endif // SERIALPORTENGINE_P_UNIX_H
