#ifndef SERIALPORTNOTIFIER_P_H
#define SERIALPORTNOTIFIER_P_H

#include "abstractserialportnotifier_p.h"
#include <QtCore/QEvent>

#if defined (Q_OS_WIN)
#  include <QtCore/private/qwineventnotifier_p.h>
#else
#  include <QtCore/QThread>
class QSocketNotifier;
#endif

class SerialPortPrivate;

#if defined (Q_OS_WIN)
class SerialPortNotifier : public QWinEventNotifier, public AbstractSerialPortNotifier
{
public:
    SerialPortNotifier(SerialPortPrivate *parent);
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
    SerialPortNotifier(SerialPortPrivate *parent);
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
