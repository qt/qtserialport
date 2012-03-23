/*
    License...
*/

#ifndef SERIALPORT_P_H
#define SERIALPORT_P_H

#include "serialport.h"
#include <QtCore/private/qringbuffer_p.h>

QT_BEGIN_NAMESPACE_SERIALPORT

class SerialPortEngine;

// General port parameters (for any OS).
class SerialPortOptions
{
public:
    SerialPortOptions()
        : inputRate(SerialPort::UnknownRate)
        , outputRate(SerialPort::UnknownRate)
        , dataBits(SerialPort::UnknownDataBits)
        , parity(SerialPort::UnknownParity)
        , stopBits(SerialPort::UnknownStopBits)
        , flow(SerialPort::UnknownFlowControl)
        , policy(SerialPort::IgnorePolicy)
        , restoreSettingsOnClose(true)
    {}

    QString systemLocation;
    qint32 inputRate;
    qint32 outputRate;
    SerialPort::DataBits dataBits;
    SerialPort::Parity parity;
    SerialPort::StopBits stopBits;
    SerialPort::FlowControl flow;
    SerialPort::DataErrorPolicy policy;
    bool restoreSettingsOnClose;
};

class SerialPortPrivate
{
    Q_DECLARE_PUBLIC(SerialPort)
public:
    SerialPortPrivate(SerialPort *parent);
    virtual ~SerialPortPrivate();

    void setPort(const QString &port);
    QString port() const;

    bool open(QIODevice::OpenMode mode);
    void close();

    bool setRate(qint32 rate, SerialPort::Directions dir);
    qint32 rate(SerialPort::Directions dir) const;

    bool setDataBits(SerialPort::DataBits dataBits);
    SerialPort::DataBits dataBits() const;

    bool setParity(SerialPort::Parity parity);
    SerialPort::Parity parity() const;

    bool setStopBits(SerialPort::StopBits stopBits);
    SerialPort::StopBits stopBits() const;

    bool setFlowControl(SerialPort::FlowControl flow);
    SerialPort::FlowControl flowControl() const;

    bool dtr() const;
    bool rts() const;

    bool setDtr(bool set);
    bool setRts(bool set);

    SerialPort::Lines lines() const;

    bool flush();
    bool reset();

    bool sendBreak(int duration);
    bool setBreak(bool set);

    bool setDataErrorPolicy(SerialPort::DataErrorPolicy policy);
    SerialPort::DataErrorPolicy dataErrorPolicy() const;

    SerialPort::PortError error() const;
    void unsetError();
    void setError(SerialPort::PortError error);

    qint64 bytesAvailable() const;
    qint64 bytesToWrite() const;

    qint64 read(char *data, qint64 len);
    qint64 write(const char *data, qint64 len);
    bool waitForReadOrWrite(int timeout,
                            bool checkRead, bool checkWrite,
                            bool *selectForRead, bool *selectForWrite);

    void clearBuffers();
    bool readFromPort();

    bool canReadNotification();
    bool canWriteNotification();
    bool canErrorNotification();

public:
    qint64 readBufferMaxSize;
    QRingBuffer readBuffer;
    QRingBuffer writeBuffer;
    bool isBuffered;

    bool readSerialNotifierCalled;
    bool readSerialNotifierState;
    bool readSerialNotifierStateSet;
    bool emittedReadyRead;
    bool emittedBytesWritten;

    SerialPort::PortError portError;

    SerialPortEngine *engine;

    SerialPortOptions options;

private:
    SerialPort * const q_ptr;
};

QT_END_NAMESPACE_SERIALPORT

#endif // SERIALPORT_P_H
