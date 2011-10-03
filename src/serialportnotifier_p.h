/*
    License...
*/

#ifndef SERIALPORTNOTIFIER_P_H
#define SERIALPORTNOTIFIER_P_H

#include <QtCore/QEvent>

#if defined (Q_OS_WIN)
#  include <qt_windows.h>
#  if defined (Q_OS_WINCE)
#    include <QtCore/QThread>
#    include <QtCore/QMutex>
#  else
#    include <QtCore/private/qwineventnotifier_p.h>
#  endif
#elif defined (Q_OS_SYMBIAN)
#  include <QtCore/QObject>
#else
#  include <QtCore/QThread>
class QSocketNotifier;
#endif

QT_BEGIN_NAMESPACE

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

#if defined (Q_OS_WIN)
#  if defined (Q_OS_WINCE)
class SerialPortNotifier : public QThread, public AbstractSerialPortNotifier
#  else
class SerialPortNotifier : public QWinEventNotifier, public AbstractSerialPortNotifier
#  endif
{
    Q_OBJECT
public:
    explicit SerialPortNotifier(QObject *parent = 0);
    virtual ~SerialPortNotifier();

    virtual bool isReadNotificationEnabled() const;
    virtual void setReadNotificationEnabled(bool enable);
    virtual bool isWriteNotificationEnabled() const;
    virtual void setWriteNotificationEnabled(bool enable);

protected:
#  if defined (Q_OS_WINCE)
    virtual void run();
#  else
    virtual bool event(QEvent *e);
#  endif

private:
    DWORD m_currentMask;
    DWORD m_setMask;
#  if defined (Q_OS_WINCE)
    volatile bool m_running;
    QMutex m_setCommMaskMutex;
#  else
    OVERLAPPED m_ov;

    void setMaskAndActivateEvent();
#  endif
};
#elif defined (Q_OS_SYMBIAN)
class SerialPortNotifier : public QObject, public AbstractSerialPortNotifier /* public CActive ?? */
{
    Q_OBJECT
public:
    explicit SerialPortNotifier(QObject *parent = 0);
    virtual ~SerialPortNotifier();

    virtual bool isReadNotificationEnabled() const;
    virtual void setReadNotificationEnabled(bool enable);
    virtual bool isWriteNotificationEnabled() const;
    virtual void setWriteNotificationEnabled(bool enable);
};
#else
class SerialPortNotifier : public QObject, public AbstractSerialPortNotifier
{
    Q_OBJECT
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
    QSocketNotifier *m_exceptionNotifier;
};
#endif

QT_END_NAMESPACE

#endif // SERIALPORTNOTIFIER_P_H
