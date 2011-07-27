/*
    License...
*/

#include "serialportnotifier_p.h"
#include "serialport_p.h"

#include <QtCore/QSocketNotifier>


/* Public methods */


SerialPortNotifier::SerialPortNotifier(QObject *parent)
    : QObject(parent)
    , m_readNotifier(0)
    , m_writeNotifier(0)
{
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
            m_readNotifier = new QSocketNotifier(m_ref->m_descriptor, QSocketNotifier::Read, this);
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
            m_writeNotifier = new QSocketNotifier(m_ref->m_descriptor, QSocketNotifier::Write, this);
            m_writeNotifier->installEventFilter(this);
            m_writeNotifier->setEnabled(true);
        }
    }
}


/* Protected methods */


bool SerialPortNotifier::eventFilter(QObject *obj, QEvent *e)
{
    if (m_ref) {
        if ((obj == m_readNotifier) && (e->type() == QEvent::SockAct)) {
            m_ref->canReadNotification();
            return true;
        }
        if ((obj == m_writeNotifier) && (e->type() == QEvent::SockAct)) {
            m_ref->canWriteNotification();
            return true;
        }
    }
    return SerialPortNotifier::eventFilter(obj, e);
}

