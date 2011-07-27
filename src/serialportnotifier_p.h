/*
    License...
*/

#ifndef SERIALPORTNOTIFIER_P_H
#define SERIALPORTNOTIFIER_P_H

class SerialPortPrivate;

// Serial port notifier interface.
class AbstractSerialPortNotifier
{
public:
    AbstractSerialPortNotifier() : m_ref(0) {}
    void setRef(SerialPortPrivate *ref) { m_ref = ref; }

    virtual bool isReadNotificationEnabled() const = 0;
    virtual void setReadNotificationEnabled(bool enable) = 0;
    virtual bool isWriteNotificationEnabled() const = 0;
    virtual void setWriteNotificationEnabled(bool enable) = 0;

protected:
    SerialPortPrivate *m_ref;
};


#include <QtCore/QEvent>

#if defined (Q_OS_WIN)
#  include <QtCore/private/qwineventnotifier_p.h>
#  include <qt_windows.h>
#else
#  include <QtCore/QThread>
class QSocketNotifier;
#endif


#if defined (Q_OS_WIN)
class SerialPortNotifier : public QWinEventNotifier, public AbstractSerialPortNotifier
{
public:
    explicit SerialPortNotifier(QObject *parent = 0);
    virtual ~SerialPortNotifier();

    virtual bool isReadNotificationEnabled() const;
    virtual void setReadNotificationEnabled(bool enable);
    virtual bool isWriteNotificationEnabled() const;
    virtual void setWriteNotificationEnabled(bool enable);

protected:
    virtual bool event(QEvent *e);

private:
    OVERLAPPED m_ov;
    DWORD m_currentMask;
    DWORD m_setMask;

    void setMaskAndActivateEvent();
};
#else
class SerialPortNotifier : public QObject, public AbstractSerialPortNotifier
{
public:
    explicit SerialPortNotifier(QObject *parent = 0);
    virtual ~SerialPortNotifier();

    virtual bool isReadNotificationEnabled() const;
    virtual void setReadNotificationEnabled(bool enable);
    virtual bool isWriteNotificationEnabled() const;
    virtual void setWriteNotificationEnabled(bool enable);

protected:
    virtual bool eventFilter(QObject *obj, QEvent *e);

private:
    QSocketNotifier *m_readNotifier;
    QSocketNotifier *m_writeNotifier;
};
#endif

#endif // SERIALPORTNOTIFIER_P_H
