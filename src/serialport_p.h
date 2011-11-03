/*
    License...
*/

#ifndef SERIALPORT_P_H
#define SERIALPORT_P_H

#include "serialport.h"
#include "ringbuffer_p.h"

QT_BEGIN_NAMESPACE

class SerialPortEngine;

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

    qint64 m_readBufferMaxSize;
    RingBuffer m_readBuffer;
    RingBuffer m_writeBuffer;
    bool m_isBuffered;
    bool m_readSerialNotifierCalled;
    bool m_readSerialNotifierState;
    bool m_readSerialNotifierStateSet;
    bool m_emittedReadyRead;
    bool m_emittedBytesWritten;

    SerialPortEngine *m_engine;

    // General port parameters (for any OS).
    SerialPort *q_ptr;
    QString m_systemLocation;
    qint32 m_inRate;
    qint32 m_outRate;
    SerialPort::DataBits m_dataBits;
    SerialPort::Parity m_parity;
    SerialPort::StopBits m_stopBits;
    SerialPort::FlowControl m_flow;
    SerialPort::DataErrorPolicy m_policy;
    SerialPort::PortError m_portError;
    bool m_restoreSettingsOnClose;
};

QT_END_NAMESPACE

#endif // SERIALPORT_P_H
