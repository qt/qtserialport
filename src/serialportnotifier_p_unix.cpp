#include "serialportnotifier_p.h"
#include "serialport_p.h"

#include <QtCore/QSocketNotifier>

SerialPortNotifier::SerialPortNotifier(SerialPortPrivate *parent)
        : QObject(parent->q_ptr)
        , m_readNotifier(0)
        , m_writeNotifier(0)
{
    m_parent = parent;
}

SerialPortNotifier::~SerialPortNotifier()
{
    if (m_readNotifier)
        m_readNotifier->setEnabled(false);
    if (m_writeNotifier)
        m_writeNotifier->setEnabled(false);
}

bool SerialPortNotifier::isReadNotificationEnabled() const
{
    return (m_readNotifier && m_readNotifier->isEnabled());
}

void SerialPortNotifier::setReadNotificationEnabled(bool enable)
{
    if (m_readNotifier) {
        m_readNotifier->setEnabled(enable);
    }
    else {
        if (enable) {
            m_readNotifier = new QSocketNotifier(m_parent->descriptor(), QSocketNotifier::Read, this);
            m_readNotifier->installEventFilter(this);
            m_readNotifier->setEnabled(true);
        }
    }
}

bool SerialPortNotifier::isWriteNotificationEnabled() const
{
    return (m_writeNotifier && m_writeNotifier->isEnabled());
}

void SerialPortNotifier::setWriteNotificationEnabled(bool enable)
{
    if (m_writeNotifier) {
        m_writeNotifier->setEnabled(enable);
    }
    else {
        if (enable) {
            m_writeNotifier = new QSocketNotifier(m_parent->descriptor(), QSocketNotifier::Write, this);
            m_writeNotifier->installEventFilter(this);
            m_writeNotifier->setEnabled(true);
        }
    }
}

bool SerialPortNotifier::eventFilter(QObject *obj, QEvent *e)
{
    if ((obj == m_readNotifier) && (e->type() == QEvent::SockAct)) {
        m_parent->readNotification();
        return true;
    }
    if ((obj == m_writeNotifier) && (e->type() == QEvent::SockAct)) {
        m_parent->writeNotification();
        return true;
    }
    return SerialPortNotifier::eventFilter(obj, e);
}

