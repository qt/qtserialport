/*
    License...
*/

#ifndef SERIALPORTENGINE_P_SYMBIAN_H
#define SERIALPORTENGINE_P_SYMBIAN_H

#include "serialport.h"
#include "serialportengine_p.h"

#include <c32comm.h>

QT_BEGIN_NAMESPACE

class SymbianSerialPortEngine : public QObject, public SerialPortEngine
{
    Q_OBJECT
public:
    SymbianSerialPortEngine(SerialPortPrivate *parent);
    virtual ~SymbianSerialPortEngine();

    virtual bool nativeOpen(const QString &location, QIODevice::OpenMode mode);
    virtual void nativeClose(const QString &location);

    virtual SerialPort::Lines nativeLines() const;

    virtual bool setNativeDtr(bool set);
    virtual bool setNativeRts(bool set);

    virtual bool nativeFlush();
    virtual bool nativeReset();

    virtual bool nativeSendBreak(int duration);
    virtual bool nativeSetBreak(bool set);

    virtual qint64 nativeBytesAvailable() const;
    virtual qint64 nativeBytesToWrite() const;

    virtual qint64 nativeRead(char *data, qint64 len);
    virtual qint64 nativeWrite(const char *data, qint64 len);
    virtual bool nativeSelect(int timeout,
                              bool checkRead, bool checkWrite,
                              bool *selectForRead, bool *selectForWrite);

    virtual QString nativeToSystemLocation(const QString &port) const;
    virtual QString nativeFromSystemLocation(const QString &location) const;

    virtual bool setNativeRate(qint32 rate, SerialPort::Directions dir);
    virtual bool setNativeDataBits(SerialPort::DataBits dataBits);
    virtual bool setNativeParity(SerialPort::Parity parity);
    virtual bool setNativeStopBits(SerialPort::StopBits stopBits);
    virtual bool setNativeFlowControl(SerialPort::FlowControl flowControl);

    virtual bool setNativeDataInterval(int usecs);
    virtual bool setNativeReadTimeout(int msecs);

    virtual bool setNativeDataErrorPolicy(SerialPort::DataErrorPolicy policy);

    virtual bool isReadNotificationEnabled() const;
    virtual void setReadNotificationEnabled(bool enable);
    virtual bool isWriteNotificationEnabled() const;
    virtual void setWriteNotificationEnabled(bool enable);

    virtual bool processNativeIOErrors();

protected:
    virtual void detectDefaultSettings();
    virtual bool saveOldsettings();
    virtual bool restoreOldsettings();
    virtual void prepareOtherOptions();

    //virtual bool eventFilter(QObject *obj, QEvent *e);

private:
    TCommConfig m_currSettings;
    TCommConfig m_oldSettings;
    RComm m_descriptor;
    mutable RTimer m_selectTimer;

    bool updateCommConfig();

    bool isRestrictedAreaSettings(SerialPort::DataBits dataBits,
                                  SerialPort::StopBits stopBits) const;
};

QT_END_NAMESPACE

#endif // SERIALPORTENGINE_P_SYMBIAN_H
