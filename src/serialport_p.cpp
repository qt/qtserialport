/*
    License...
*/

#include "serialport_p.h"
#include "abstractserialportnotifier_p.h"

#include <QtCore/QAbstractEventDispatcher>

bool SerialPortPrivate::canReadNotification()
{
    Q_Q(SerialPort);

    // Prevent recursive calls
    if (m_readSerialNotifierCalled) {
        if (!m_readSerialNotifierStateSet) {
            m_readSerialNotifierStateSet = true;
            m_readSerialNotifierState = isReadNotificationEnabled();
            setReadNotificationEnabled(false);
        }
    }
    m_readSerialNotifierCalled = true;

    //if (!m_isBuffered)
    //    this->serialEngine->setReadNotificationEnabled(false);

    // If buffered, read data from the serial into the read buffer
    qint64 newBytes = 0;
    if (m_isBuffered) {
        // Return if there is no space in the buffer
        if (m_readBufferMaxSize
                && (m_readBuffer.size() >= m_readBufferMaxSize)) {

            m_readSerialNotifierCalled = false;
            return false;
        }

        // If reading from the serial fails after getting a read
        // notification, close the serial.
        newBytes = m_readBuffer.size();

        if (!readFromSerial()) {
            m_readSerialNotifierCalled = false;
            return false;
        }
        newBytes = m_readBuffer.size() - newBytes;

        // If read buffer is full, disable the read serial notifier.
        if (m_readBufferMaxSize
                && (m_readBuffer.size() == m_readBufferMaxSize)) {

            setReadNotificationEnabled(false);
        }
    }

    // only emit readyRead() when not recursing, and only if there is data available
    bool hasData = (m_isBuffered) ? (newBytes > 0) : (bytesAvailable() > 0);

    if ((!m_emittedReadyRead) && hasData) {
        m_emittedReadyRead = true;
        emit q->readyRead();
        m_emittedReadyRead = false;
    }

    if ((!hasData) && isReadNotificationEnabled())
        setReadNotificationEnabled(true);

    // reset the read serial notifier state if we reentered inside the
    // readyRead() connected slot.
    if (m_readSerialNotifierStateSet &&
            (m_readSerialNotifierState != isReadNotificationEnabled())) {

        setReadNotificationEnabled(m_readSerialNotifierState);
        m_readSerialNotifierStateSet = false;
    }
    m_readSerialNotifierCalled = false;
    return true;
}

bool SerialPortPrivate::canWriteNotification() {
#if defined (Q_OS_WIN)
    if (isWriteNotificationEnabled())
        setWriteNotificationEnabled(false);
#endif

    int tmp = m_writeBuffer.size();
    flush();

#if defined (Q_OS_WIN)
    if (m_writeBuffer.isEmpty())
        setWriteNotificationEnabled(true);
#else
    if (m_writeBuffer.isEmpty())
        setWriteNotificationEnabled(false);
#endif
    return (m_writeBuffer.size() < tmp);
}

bool SerialPortPrivate::isReadNotificationEnabled() const
{
    return (m_notifier
            && m_notifier->isReadNotificationEnabled());
}

void SerialPortPrivate::setReadNotificationEnabled(bool enable, bool onClose)
{
    if (onClose)
        enable = false;

    if (!m_notifier && enable)
            m_notifier = AbstractSerialPortNotifier::createNotifier(this);

    if (m_notifier)
        m_notifier->setReadNotificationEnabled(enable);
    if (onClose && m_notifier)
        clearNotification();
}

bool SerialPortPrivate::isWriteNotificationEnabled() const
{
    return (m_notifier
            && m_notifier->isWriteNotificationEnabled());
}

void SerialPortPrivate::setWriteNotificationEnabled(bool enable, bool onClose)
{
    if (onClose)
        enable = false;

    if (!m_notifier && enable)
            m_notifier = AbstractSerialPortNotifier::createNotifier(this);

    if (m_notifier)
        m_notifier->setWriteNotificationEnabled(enable);
    if (onClose && m_notifier)
        clearNotification();
}

void SerialPortPrivate::clearNotification()
{
    AbstractSerialPortNotifier::deleteNotifier(m_notifier);
    m_notifier = 0;
}

void SerialPortPrivate::clearBuffers()
{
    m_writeBuffer.clear();
    m_readBuffer.clear();
}

bool SerialPortPrivate::readFromSerial()
{
    qint64 bytesToRead = bytesAvailable();

    if (bytesToRead <= 0)
        return false;

    if (m_readBufferMaxSize
        && (bytesToRead > (m_readBufferMaxSize - m_readBuffer.size()))) {

        bytesToRead = m_readBufferMaxSize - m_readBuffer.size();
    }

    char *ptr = m_readBuffer.reserve(bytesToRead);
    qint64 readBytes = read(ptr, bytesToRead);

    if (readBytes <= 0) {
        m_readBuffer.chop(bytesToRead);
        return false;
    }
    m_readBuffer.chop(int(bytesToRead - ((readBytes < 0) ? qint64(0) : readBytes)));
    return true;
}

