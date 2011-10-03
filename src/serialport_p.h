/*
    License...
*/

#ifndef SERIALPORT_P_H
#define SERIALPORT_P_H

#include "serialport.h"
#include "abstractserialport_p.h"
#include "serialportnotifier_p.h"

#if defined (Q_OS_WIN)
#  include <qt_windows.h>
#  if defined (Q_OS_WINCE)
#    include <QtCore/QMutex>
#  endif
#elif defined (Q_OS_SYMBIAN)
#  include <c32comm.h>
#else
#  include <termios.h>
//#  undef CMSPAR
#endif

#if defined (Q_OS_WINCE)
#  include <QtCore/QThread>
#  include <QtCore/QTimer>

class WinCeWaitCommEventBreaker : public QThread
{
    Q_OBJECT
public:
    WinCeWaitCommEventBreaker(HANDLE descriptor, int timeout, QObject *parent = 0)
        : QThread(parent), m_descriptor(descriptor),
          m_timeout(timeout), m_worked(false) {
        start();
    }
	virtual ~WinCeWaitCommEventBreaker() {
		stop();
		wait();
	}
    void stop() { exit(0); }
    bool isWorked() const { return m_worked; }

protected:
    void run() {
        QTimer timer;
		QObject::connect(&timer, SIGNAL(timeout()), this, SLOT(procTimeout()), Qt::DirectConnection);
        timer.start(m_timeout);
        exec();
        m_worked = true;
    }

private slots:
    void procTimeout() {
        ::SetCommMask(m_descriptor, 0);
        stop();
    }

private:
    HANDLE m_descriptor;
    int m_timeout;
    volatile bool m_worked;
};
#endif

class SerialPortPrivate : public AbstractSerialPortPrivate
{
    Q_DECLARE_PUBLIC(SerialPort)
public:
    SerialPortPrivate(SerialPort *parent);

    virtual bool open(QIODevice::OpenMode mode);
    virtual void close();

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
    virtual bool waitForReadOrWrite(int timeout,
                                    bool checkRead, bool checkWrite,
                                    bool *selectForRead, bool *selectForWrite);

protected:
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

    virtual bool nativeFlush();

    virtual void detectDefaultSettings();

    virtual bool saveOldsettings();
    virtual bool restoreOldsettings();

private:
#if defined (Q_OS_WIN)
    enum CommTimeouts {
        ReadIntervalTimeout, ReadTotalTimeoutMultiplier,
        ReadTotalTimeoutConstant, WriteTotalTimeoutMultiplier,
        WriteTotalTimeoutConstant
    };

    DCB m_currDCB;
    DCB m_oldDCB;
    COMMTIMEOUTS m_currCommTimeouts;
    COMMTIMEOUTS m_oldCommTimeouts;
    HANDLE m_descriptor;
    bool m_flagErrorFromCommEvent;

#  if defined (Q_OS_WINCE)
    QMutex m_readNotificationMutex;
    QMutex m_writeNotificationMutex;
    QMutex m_errorNotificationMutex;
    QMutex m_settingsChangeMutex;
#  else
    OVERLAPPED m_ovRead;
    OVERLAPPED m_ovWrite;
    OVERLAPPED m_ovSelect;

    bool createEvents(bool rx, bool tx);
    void closeEvents() const;
#  endif

    void recalcTotalReadTimeoutConstant();
    void prepareCommTimeouts(CommTimeouts cto, DWORD msecs);
    bool updateDcb();
    bool updateCommTimeouts();

    bool isRestrictedAreaSettings(SerialPort::DataBits dataBits,
                                  SerialPort::StopBits stopBits) const;

    bool processCommEventError();
#elif defined (Q_OS_SYMBIAN)
    TCommConfig m_currSettings;
    TCommConfig m_oldSettings;

    RComm m_descriptor;

    bool updateCommConfig();
    bool isRestrictedAreaSettings(SerialPort::DataBits dataBits,
                                  SerialPort::StopBits stopBits) const;
#else
    struct termios m_currTermios;
    struct termios m_oldTermios;

    int m_descriptor;

    void prepareTimeouts(int msecs);
    bool updateTermious();
    bool setStandartRate(SerialPort::Directions dir, speed_t rate);
    bool setCustomRate(qint32 rate);

    bool isRestrictedAreaSettings(SerialPort::DataBits dataBits,
                                  SerialPort::StopBits stopBits) const;
#  if !defined (CMSPAR)
    qint64 writePerChar(const char *data, qint64 maxSize);
    qint64 readPerChar(char *data, qint64 maxSize);
#  endif 

#endif
    friend class SerialPortNotifier;
    SerialPortNotifier m_notifier;

    bool canReadNotification();
    bool canWriteNotification();
    bool canErrorNotification();

    bool isReadNotificationEnabled() const;
    void setReadNotificationEnabled(bool enable);
    bool isWriteNotificationEnabled() const;
    void setWriteNotificationEnabled(bool enable);

    void clearBuffers();
    bool readFromPort();

    void prepareOtherOptions();
};

#endif // SERIALPORT_P_H
