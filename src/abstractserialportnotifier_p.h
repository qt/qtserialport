#ifndef ABSTRACTSERIALPORTNOTIFIER_P_H
#define ABSTRACTSERIALPORTNOTIFIER_P_H

class SerialPortPrivate;

class AbstractSerialPortNotifier
{
public:
    static AbstractSerialPortNotifier *sreateNotifier(SerialPortPrivate *parent);
    static void deleteNotifier(AbstractSerialPortNotifier *notifier);

    virtual bool isReadNotificationEnabled() const = 0;
    virtual void setReadNotificationEnabled(bool enable) = 0;
    virtual bool isWriteNotificationEnabled() const = 0;
    virtual void setWriteNotificationEnabled(bool enable) = 0;

protected:
    SerialPortPrivate *m_parent;
};


#endif // ABSTRACTSERIALPORTNOTIFIER_P_H
