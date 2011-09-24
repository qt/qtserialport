/*
    License...
*/

#include "serialportnotifier_p.h"
#include "serialport_p.h"

//#include <QtCore/QDebug>


/* Public methods */


#if defined (Q_OS_WINCE)
SerialPortNotifier::SerialPortNotifier(QObject *parent)
    : QThread(parent)
    , m_currentMask(0)
    , m_setMask(EV_ERR)
    , m_running(true)
{
    //??
}

SerialPortNotifier::~SerialPortNotifier()
{
    m_running = false;
    Q_ASSERT(m_ref);
    ::SetCommMask(m_ref->m_descriptor, 0);
    //terminate();
    wait();
}
#else
SerialPortNotifier::SerialPortNotifier(QObject *parent)
    : QWinEventNotifier(parent)
    , m_currentMask(0)
    , m_setMask(EV_ERR)
{
    ::memset(&m_ov, 0, sizeof(OVERLAPPED));
    m_ov.hEvent = ::CreateEvent(0, false, false, 0);
    Q_ASSERT(m_ov.hEvent);
    setHandle(m_ov.hEvent);
}

SerialPortNotifier::~SerialPortNotifier()
{
    setEnabled(false);
    Q_ASSERT(m_ov.hEvent);
    ::CloseHandle(m_ov.hEvent);
}
#endif

bool SerialPortNotifier::isReadNotificationEnabled() const
{
#if defined (Q_OS_WINCE)
    bool flag = isRunning();
#else
    bool flag = isEnabled();
#endif
    return (flag && (m_setMask & EV_RXCHAR));
}

void SerialPortNotifier::setReadNotificationEnabled(bool enable)
{
    Q_ASSERT(m_ref);
#if defined (Q_OS_WINCE)
    m_setCommMaskMutex.lock();
    ::GetCommMask(m_ref->m_descriptor, &m_currentMask);
#endif

    if (enable)
        m_setMask |= EV_RXCHAR;
    else
        m_setMask &= ~EV_RXCHAR;

#if defined (Q_OS_WINCE)
    if (m_setMask != m_currentMask)
        ::SetCommMask(m_ref->m_descriptor, m_setMask);

    m_setCommMaskMutex.unlock();

    if (enable && !isRunning())
        start();
#else
    setMaskAndActivateEvent();
#endif
}

bool SerialPortNotifier::isWriteNotificationEnabled() const
{
#if defined (Q_OS_WINCE)
    bool flag = isRunning();
#else
    bool flag = isEnabled();
#endif
    return (flag && (m_setMask & EV_TXEMPTY));
}

void SerialPortNotifier::setWriteNotificationEnabled(bool enable)
{
    Q_ASSERT(m_ref);
#if defined (Q_OS_WINCE)
    m_setCommMaskMutex.lock();
    ::GetCommMask(m_ref->m_descriptor, &m_currentMask);
#endif

    if (enable)
        m_setMask |= EV_TXEMPTY;
    else
        m_setMask &= ~EV_TXEMPTY;

#if defined (Q_OS_WINCE)
    if (m_setMask != m_currentMask)
        ::SetCommMask(m_ref->m_descriptor, m_setMask);

    m_setCommMaskMutex.unlock();

    if (enable && !isRunning())
        start();
#else
    setMaskAndActivateEvent();
#endif
    // This only for OS Windows, as EV_TXEMPTY event is triggered only
    // after the last byte of data (as opposed to events such as Write QSocketNotifier).
    // Therefore, we are forced to run writeNotification(), as EV_TXEMPTY does not work.
    if (enable && m_ref)
        m_ref->canWriteNotification();
}


/* Protected methods */


#if defined (Q_OS_WINCE)
void SerialPortNotifier::run()
{
    Q_ASSERT(m_ref);
    while (m_running) {

        m_setCommMaskMutex.lock();
        ::SetCommMask(m_ref->m_descriptor, m_setMask);
        m_setCommMaskMutex.unlock();

        if (::WaitCommEvent(m_ref->m_descriptor, &m_currentMask, 0) != 0) {

            // Wait until complete the operation changes the port settings.
            m_ref->m_settingsChangeMutex.lock();
            m_ref->m_settingsChangeMutex.unlock();

            if (EV_ERR & m_currentMask & m_setMask) {
                m_ref->canErrorNotification();
            }
            if (EV_RXCHAR & m_currentMask & m_setMask) {
                m_ref->canReadNotification();
            }
            //FIXME: This is why it does not work?
            if (EV_TXEMPTY & m_currentMask & m_setMask) {
                m_ref->canWriteNotification();
            }
        }
    }
}
#else
bool SerialPortNotifier::event(QEvent *e)
{
    Q_ASSERT(m_ref);
    bool ret = false;
    if (e->type() == QEvent::WinEventAct) {
        if (EV_ERR & m_currentMask & m_setMask) {
            m_ref->canErrorNotification();
            ret = true;
        }
        if (EV_RXCHAR & m_currentMask & m_setMask) {
            m_ref->canReadNotification();
            ret = true;
        }
        //FIXME: This is why it does not work?
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
#endif


/* Private methods */


#if !defined (Q_OS_WINCE)
void SerialPortNotifier::setMaskAndActivateEvent()
{
    Q_ASSERT(m_ref);
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
#endif
