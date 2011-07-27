/*
    License...
*/

#include "serialportnotifier_p.h"
#include "serialport_p.h"


/* Public methods */


SerialPortNotifier::SerialPortNotifier(QObject *parent)
        : QWinEventNotifier(parent)
        , m_currentMask(0)
        , m_setMask(0)
{
    ::memset(&m_ov, 0, sizeof(OVERLAPPED));
    m_ov.hEvent = ::CreateEvent(0, false, false, 0);
    setHandle(m_ov.hEvent);
}

SerialPortNotifier::~SerialPortNotifier()
{
    setEnabled(false);
    ::CloseHandle(m_ov.hEvent);
}

bool SerialPortNotifier::isReadNotificationEnabled() const
{
    return (isEnabled() && (m_setMask & EV_RXCHAR));
}

void SerialPortNotifier::setReadNotificationEnabled(bool enable)
{
    if (enable)
        m_setMask |= EV_RXCHAR;
    else
        m_setMask &= ~EV_RXCHAR;

    setMaskAndActivateEvent();
}

bool SerialPortNotifier::isWriteNotificationEnabled() const
{
    return (isEnabled() && (m_setMask & EV_TXEMPTY));
}

void SerialPortNotifier::setWriteNotificationEnabled(bool enable)
{
    if (enable)
        m_setMask |= EV_TXEMPTY;
    else
        m_setMask &= ~EV_TXEMPTY;

    setMaskAndActivateEvent();

    // This only for OS Windows, as EV_TXEMPTY event is triggered only
    // after the last byte of data (as opposed to events such as Write QSocketNotifier).
    // Therefore, we are forced to run writeNotification(), as EV_TXEMPTY does not work.
    if (enable && m_ref)
        m_ref->canWriteNotification();
}


/* Protected methods */


bool SerialPortNotifier::event(QEvent *e)
{
    bool ret = false;
    if (m_ref && (e->type() == QEvent::WinEventAct)) {
        if (EV_RXCHAR & m_currentMask & m_setMask) {
            m_ref->canReadNotification();
            ret = true;
        }
        //TODO: This is why it does not work?
        if (EV_TXEMPTY & m_currentMask & m_setMask) {
            m_ref->canWriteNotification();
            ret = true;
        }
    }
    else
        ret = QWinEventNotifier::event(e);

    ::WaitCommEvent(m_ref->m_descriptor, &m_currentMask, &m_ov);
    return ret;
}


/* Private methods */


void SerialPortNotifier::setMaskAndActivateEvent()
{
    ::SetCommMask(m_ref->m_descriptor, m_setMask);

    if (m_setMask)
        ::WaitCommEvent(m_ref->m_descriptor, &m_currentMask, &m_ov);

    switch (m_setMask) {
    case 0:
        if (isEnabled())
            setEnabled(false);
        break;
    default:
        if (!isEnabled())
            setEnabled(true);
    }
}
