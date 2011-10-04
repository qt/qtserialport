/*
    License...
*/

#include "serialportnotifier_p.h"
#include "serialport_p.h"

#include <QtCore/qsocketnotifier.h>

QT_USE_NAMESPACE

/* Public methods */


SerialPortNotifier::SerialPortNotifier(QObject *parent)
    : QObject(parent)
    , m_readNotifier(0)
    , m_writeNotifier(0)
    , m_exceptionNotifier(0)
{
}

SerialPortNotifier::~SerialPortNotifier()
{
    if (m_readNotifier)
        m_readNotifier->setEnabled(false);
    if (m_writeNotifier)
        m_writeNotifier->setEnabled(false);
    if (m_exceptionNotifier)
        m_exceptionNotifier->setEnabled(false);
}

bool SerialPortNotifier::isReadNotificationEnabled() const
{
    return (m_readNotifier && m_readNotifier->isEnabled());
}

void SerialPortNotifier::setReadNotificationEnabled(bool enable)
{
    Q_ASSERT(m_ref);
    if (m_readNotifier)
        m_readNotifier->setEnabled(enable);
    else if (enable) {
        m_readNotifier = new QSocketNotifier(m_ref->m_descriptor, QSocketNotifier::Read, this);
        m_readNotifier->installEventFilter(this);
        m_readNotifier->setEnabled(true);
    }
}

bool SerialPortNotifier::isWriteNotificationEnabled() const
{
    return (m_writeNotifier && m_writeNotifier->isEnabled());
}

void SerialPortNotifier::setWriteNotificationEnabled(bool enable)
{
    Q_ASSERT(m_ref);
    if (m_writeNotifier)
        m_writeNotifier->setEnabled(enable);
    else if (enable) {
        m_writeNotifier = new QSocketNotifier(m_ref->m_descriptor, QSocketNotifier::Write, this);
        m_writeNotifier->installEventFilter(this);
        m_writeNotifier->setEnabled(true);
    }
}


/* Protected methods */


bool SerialPortNotifier::eventFilter(QObject *obj, QEvent *e)
{
    Q_ASSERT(m_ref);
    if (e->type() == QEvent::SockAct) {
        if (obj == m_readNotifier) {
            m_ref->canReadNotification();
            return true;
        }
        if (obj == m_writeNotifier) {
            m_ref->canWriteNotification();
            return true;
        }
        if (obj == m_exceptionNotifier) {
            m_ref->canErrorNotification();
            return true;
        }
    }
    return QObject::eventFilter(obj, e);
}

