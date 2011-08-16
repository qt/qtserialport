/*
    License...
*/

#include "serialport_p.h"


/* Public methods */

bool SerialPortPrivate::flush()
{
    Q_Q(SerialPort);

    if (m_writeBuffer.isEmpty())
        return false;

    int nextSize = m_writeBuffer.nextDataBlockSize();
    const char *ptr = m_writeBuffer.readPointer();

    // Attempt to write it all in one chunk.
    qint64 written = write(ptr, nextSize);
    if (written < 0) {
        m_writeBuffer.clear();
        return false;
    }

    // Remove what we wrote so far.
    m_writeBuffer.free(written);
    if (written > 0) {
        // Don't emit bytesWritten() recursively.
        if (!m_emittedBytesWritten) {
            m_emittedBytesWritten = true;
            emit q->bytesWritten(written);
            m_emittedBytesWritten = false;
        }
    }

    if (m_writeBuffer.isEmpty() && isWriteNotificationEnabled())
        setWriteNotificationEnabled(false);

    return true;
}


/* Private methods */


bool SerialPortPrivate::canReadNotification()
{
#if defined (Q_OS_WINCE)
    QMutexLocker locker(&m_readNotificationMutex);
#endif

    Q_Q(SerialPort);

    // Prevent recursive calls.
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

    // If buffered, read data from the serial into the read buffer.
    qint64 newBytes = 0;
    if (m_isBuffered) {
        // Return if there is no space in the buffer.
        if (m_readBufferMaxSize
                && (m_readBuffer.size() >= m_readBufferMaxSize)) {

            m_readSerialNotifierCalled = false;
            return false;
        }

        // If reading from the serial fails after getting a read
        // notification, close the serial.
        newBytes = m_readBuffer.size();

        if (!readFromPort()) {
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

    // Only emit readyRead() when not recursing, and only if there is data available.
    bool hasData = (m_isBuffered) ? (newBytes > 0) : (bytesAvailable() > 0);

    if ((!m_emittedReadyRead) && hasData) {
        m_emittedReadyRead = true;
        emit q->readyRead();
        m_emittedReadyRead = false;
    }

    if ((!hasData) && isReadNotificationEnabled())
        setReadNotificationEnabled(true);

    // Reset the read serial notifier state if we reentered inside the
    // readyRead() connected slot.
    if (m_readSerialNotifierStateSet &&
            (m_readSerialNotifierState != isReadNotificationEnabled())) {

        setReadNotificationEnabled(m_readSerialNotifierState);
        m_readSerialNotifierStateSet = false;
    }
    m_readSerialNotifierCalled = false;
    return true;
}

bool SerialPortPrivate::canWriteNotification()
{
#if defined (Q_OS_WINCE)
    QMutexLocker locker(&m_writeNotificationMutex);
#endif

#if defined (Q_OS_WIN)
    if (isWriteNotificationEnabled())
        setWriteNotificationEnabled(false);
#endif

    int tmp = m_writeBuffer.size();
    flush();

#if defined (Q_OS_WIN)
    if (!m_writeBuffer.isEmpty())
        setWriteNotificationEnabled(true);
#else
    if (m_writeBuffer.isEmpty())
        setWriteNotificationEnabled(false);
#endif
    return (m_writeBuffer.size() < tmp);
}

bool SerialPortPrivate::canErrorNotification()
{
#if defined (Q_OS_WINCE)
    QMutexLocker locker(&m_errorNotificationMutex);
#endif

#if defined (Q_OS_WIN)
    return processCommEventError();
#endif
    return true;
}

bool SerialPortPrivate::isReadNotificationEnabled() const
{
    return m_notifier.isReadNotificationEnabled();
}

void SerialPortPrivate::setReadNotificationEnabled(bool enable)
{
    m_notifier.setReadNotificationEnabled(enable);
}

bool SerialPortPrivate::isWriteNotificationEnabled() const
{
    return m_notifier.isWriteNotificationEnabled();
}

void SerialPortPrivate::setWriteNotificationEnabled(bool enable)
{
    m_notifier.setWriteNotificationEnabled(enable);
}

void SerialPortPrivate::clearBuffers()
{
    m_writeBuffer.clear();
    m_readBuffer.clear();
}

bool SerialPortPrivate::readFromPort()
{
    qint64 bytesToRead = (m_policy == SerialPort::IgnorePolicy) ?
                bytesAvailable() : 1;

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

