/*
    License...
*/

#include "serialport.h"
#include "serialportinfo.h"
#include "serialport_p.h"
#include "serialportengine_p.h"

#if QT_VERSION >= 0x040700
#  include <QtCore/qelapsedtimer.h>
#else
#  include <QtCore/qtime.h>
#endif

QT_USE_NAMESPACE

//----------------------------------------------------------------

/* Public methods */

SerialPortPrivate::SerialPortPrivate(SerialPort *parent)
    : m_readBufferMaxSize(0)
    , m_readBuffer(16384)
    , m_writeBuffer(16384)
    , m_isBuffered(false)
    , m_readSerialNotifierCalled(false)
    , m_readSerialNotifierState(false)
    , m_readSerialNotifierStateSet(false)
    , m_emittedReadyRead(false)
    , m_emittedBytesWritten(false)
    , m_engine(0)
    , q_ptr(parent)
    , m_inRate(SerialPort::UnknownRate)
    , m_outRate(SerialPort::UnknownRate)
    , m_dataBits(SerialPort::UnknownDataBits)
    , m_parity(SerialPort::UnknownParity)
    , m_stopBits(SerialPort::UnknownStopBits)
    , m_flow(SerialPort::UnknownFlowControl)
    , m_policy(SerialPort::IgnorePolicy)
    , m_portError(SerialPort::NoError)
{}

SerialPortPrivate::~SerialPortPrivate()
{
    if (m_engine)
        delete m_engine;
}

void SerialPortPrivate::setPort(const QString &port)
{
    m_systemLocation = m_engine->nativeToSystemLocation(port);
}

QString SerialPortPrivate::port() const
{
    return m_engine->nativeFromSystemLocation(m_systemLocation);
}

bool SerialPortPrivate::open(QIODevice::OpenMode mode)
{
    return m_engine->nativeOpen(m_systemLocation, mode);
}

void SerialPortPrivate::close()
{
    m_engine->nativeClose(m_systemLocation);
}

bool SerialPortPrivate::setRate(qint32 rate, SerialPort::Directions dir)
{
    if (m_engine->setNativeRate(rate, dir)) {
        if (dir & SerialPort::Input)
            m_inRate = rate;
        if (dir & SerialPort::Output)
            m_outRate = rate;
        return true;
    }
    return false;
}

qint32 SerialPortPrivate::rate(SerialPort::Directions dir) const
{
    if (dir == SerialPort::AllDirections)
        return (m_inRate == m_outRate) ? (m_inRate) : SerialPort::UnknownRate;
    return (dir & SerialPort::Input) ? (m_inRate) : (m_outRate);
}

bool SerialPortPrivate::setDataBits(SerialPort::DataBits dataBits)
{
    if (m_engine->setNativeDataBits(dataBits)) {
        m_dataBits = dataBits;
        return true;
    }
    return false;
}

SerialPort::DataBits SerialPortPrivate::dataBits() const
{
    return m_dataBits;
}

bool SerialPortPrivate::setParity(SerialPort::Parity parity)
{
    if (m_engine->setNativeParity(parity)) {
        m_parity = parity;
        return true;
    }
    return false;
}

SerialPort::Parity SerialPortPrivate::parity() const
{
    return m_parity;
}

bool SerialPortPrivate::setStopBits(SerialPort::StopBits stopBits)
{
    if (m_engine->setNativeStopBits(stopBits)) {
        m_stopBits = stopBits;
        return true;
    }
    return false;
}

SerialPort::StopBits SerialPortPrivate::stopBits() const
{
    return m_stopBits;
}

bool SerialPortPrivate::setFlowControl(SerialPort::FlowControl flow)
{
    if (m_engine->setNativeFlowControl(flow)) {
        m_flow = flow;
        return true;
    }
    return false;
}

SerialPort::FlowControl SerialPortPrivate::flowControl() const
{
    return m_flow;
}

bool SerialPortPrivate::dtr() const
{
    return m_engine->nativeLines() & SerialPort::Dtr;
}

bool SerialPortPrivate::rts() const
{
    return m_engine->nativeLines() & SerialPort::Rts;
}

bool SerialPortPrivate::setDtr(bool set)
{
    return m_engine->setNativeDtr(set);
}

bool SerialPortPrivate::setRts(bool set)
{
    return m_engine->setNativeRts(set);
}

SerialPort::Lines SerialPortPrivate::lines() const
{
    return m_engine->nativeLines();
}

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

    if (m_writeBuffer.isEmpty() && m_engine->isWriteNotificationEnabled())
        m_engine->setWriteNotificationEnabled(false);

    return true;
}

bool SerialPortPrivate::reset()
{
    return m_engine->nativeReset();
}

bool SerialPortPrivate::sendBreak(int duration)
{
    return m_engine->nativeSendBreak(duration);
}

bool SerialPortPrivate::setBreak(bool set)
{
    return m_engine->nativeSetBreak(set);
}

bool SerialPortPrivate::setDataErrorPolicy(SerialPort::DataErrorPolicy policy)
{
    if (m_engine->setNativeDataErrorPolicy(policy)) {
        m_policy = policy;
        return true;
    }
    return false;
}

SerialPort::DataErrorPolicy SerialPortPrivate::dataErrorPolicy() const
{
    return m_policy;
}

SerialPort::PortError SerialPortPrivate::error() const
{
    return m_portError;
}

void SerialPortPrivate::unsetError()
{
    m_portError = SerialPort::NoError;
}

void SerialPortPrivate::setError(SerialPort::PortError error)
{
    m_portError = error;
}

qint64 SerialPortPrivate::bytesAvailable() const
{
    return m_engine->nativeBytesAvailable();
}

qint64 SerialPortPrivate::bytesToWrite() const
{
    return m_engine->nativeBytesToWrite();
}

qint64 SerialPortPrivate::read(char *data, qint64 len)
{
    return m_engine->nativeRead(data, len);
}

qint64 SerialPortPrivate::write(const char *data, qint64 len)
{
    return m_engine->nativeWrite(data, len);
}

bool SerialPortPrivate::waitForReadOrWrite(int timeout,
                                           bool checkRead, bool checkWrite,
                                           bool *selectForRead, bool *selectForWrite)
{
    return m_engine->nativeSelect(timeout, checkRead, checkWrite, selectForRead, selectForWrite);
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

bool SerialPortPrivate::canReadNotification()
{
    Q_Q(SerialPort);

#if defined (Q_OS_WINCE)
    m_engine->lockNotification(SerialPortEngine::CanReadLocker, true);
#endif
    // Prevent recursive calls.
    if (m_readSerialNotifierCalled) {
        if (!m_readSerialNotifierStateSet) {
            m_readSerialNotifierStateSet = true;
            m_readSerialNotifierState = m_engine->isReadNotificationEnabled();
            m_engine->setReadNotificationEnabled(false);
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

            m_engine->setReadNotificationEnabled(false);
        }
    }

    // Only emit readyRead() when not recursing, and only if there is data available.
    bool hasData = (m_isBuffered) ? (newBytes > 0) : (bytesAvailable() > 0);

    if ((!m_emittedReadyRead) && hasData) {
        m_emittedReadyRead = true;
        emit q->readyRead();
        m_emittedReadyRead = false;
    }

    if ((!hasData) && m_engine->isReadNotificationEnabled())
        m_engine->setReadNotificationEnabled(true);

    // Reset the read serial notifier state if we reentered inside the
    // readyRead() connected slot.
    if (m_readSerialNotifierStateSet &&
            (m_readSerialNotifierState != m_engine->isReadNotificationEnabled())) {

        m_engine->setReadNotificationEnabled(m_readSerialNotifierState);
        m_readSerialNotifierStateSet = false;
    }
    m_readSerialNotifierCalled = false;
    return true;
}

bool SerialPortPrivate::canWriteNotification()
{
#if defined (Q_OS_WINCE)
    m_engine->lockNotification(SerialPortEngine::CanWriteLocker, true);
#endif

#if defined (Q_OS_WIN)
    if (m_engine->isWriteNotificationEnabled())
        m_engine->setWriteNotificationEnabled(false);
#endif

    int tmp = m_writeBuffer.size();
    flush();

#if defined (Q_OS_WIN)
    if (!m_writeBuffer.isEmpty())
        m_engine->setWriteNotificationEnabled(true);
#else
    if (m_writeBuffer.isEmpty())
        m_engine->setWriteNotificationEnabled(false);
#endif
    return (m_writeBuffer.size() < tmp);
}

bool SerialPortPrivate::canErrorNotification()
{
#if defined (Q_OS_WINCE)
    m_engine->lockNotification(SerialPortEngine::CanErrorLocker, true);
#endif
    return m_engine->processNativeIOErrors();
}

//----------------------------------------------------------------

/* Public methods */

SerialPort::SerialPort(QObject *parent)
    : QIODevice(parent)
    , d_ptr(new SerialPortPrivate(this))
{}

SerialPort::SerialPort(const QString &name, QObject *parent)
    : QIODevice(parent)
    , d_ptr(new SerialPortPrivate(this))
{
    setPort(name);
}

SerialPort::SerialPort(const SerialPortInfo &info, QObject *parent)
    : QIODevice(parent)
    , d_ptr(new SerialPortPrivate(this))
{
    setPort(info);
}

SerialPort::~SerialPort()
{
    /**/
    close();
    delete d_ptr;
}

void SerialPort::setPort(const QString &name)
{
    Q_D(SerialPort);
    d->setPort(name);
}

void SerialPort::setPort(const SerialPortInfo &info)
{
    Q_D(SerialPort);
    d->setPort(info.systemLocation());
}

QString SerialPort::portName() const
{
    Q_D(const SerialPort);
    return d->port();
}

bool SerialPort::open(OpenMode mode)
{
    Q_D(SerialPort);

    // Define while not supported modes.
    static OpenMode unsupportedModes = (Append | Truncate | Text);

    if ((mode & unsupportedModes) || (mode == NotOpen)) {
        d->setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }

    if (!isOpen()) {
        if (d->open(mode)) {
            QIODevice::open(mode);
            d->clearBuffers();

            if (mode & ReadOnly)
                d->m_engine->setReadNotificationEnabled(true);
            if (mode & WriteOnly)
                d->m_engine->setWriteNotificationEnabled(true);

            d->m_isBuffered = !(mode & Unbuffered);
            return QIODevice::open(mode);
        }
    }
    else
        d->setError(SerialPort::DeviceAlreadyOpenedError);

    close();
    return false;
}

void SerialPort::close()
{
    Q_D(SerialPort);
    if (isOpen()) {
        d->m_engine->setReadNotificationEnabled(false);
        d->m_engine->setWriteNotificationEnabled(false);
        d->clearBuffers();
        d->close();
        QIODevice::close();
    }
    else
        d->setError(SerialPort::DeviceIsNotOpenedError);
}

bool SerialPort::setRate(qint32 rate, Directions dir)
{
    Q_D(SerialPort);
    return d->setRate(rate, dir);
}

qint32 SerialPort::rate(Directions dir) const
{
    Q_D(const SerialPort);
    return d->rate(dir);
}

bool SerialPort::setDataBits(DataBits dataBits)
{
    Q_D(SerialPort);
    return d->setDataBits(dataBits);
}

SerialPort::DataBits SerialPort::dataBits() const
{
    Q_D(const SerialPort);
    return d->dataBits();
}

bool SerialPort::setParity(Parity parity)
{
    Q_D(SerialPort);
    return d->setParity(parity);
}

SerialPort::Parity SerialPort::parity() const
{
    Q_D(const SerialPort);
    return d->parity();
}

bool SerialPort::setStopBits(StopBits stopBits)
{
    Q_D(SerialPort);
    return d->setStopBits(stopBits);
}

SerialPort::StopBits SerialPort::stopBits() const
{
    Q_D(const SerialPort);
    return d->stopBits();
}

bool SerialPort::setFlowControl(FlowControl flow)
{
    Q_D(SerialPort);
    return d->setFlowControl(flow);
}

SerialPort::FlowControl SerialPort::flowControl() const
{
    Q_D(const SerialPort);
    return d->flowControl();
}

bool SerialPort::dtr() const
{
    Q_D(const SerialPort);
    return d->dtr();
}

bool SerialPort::rts() const
{
    Q_D(const SerialPort);
    return d->rts();
}

SerialPort::Lines SerialPort::lines() const
{
    Q_D(const SerialPort);
    return d->lines();
}

bool SerialPort::flush()
{
    Q_D(SerialPort);
    return d->flush() || d->m_engine->nativeFlush();
}

bool SerialPort::reset()
{
    Q_D(SerialPort);
    d->clearBuffers();
    return d->reset();
}

bool SerialPort::setDataErrorPolicy(DataErrorPolicy policy)
{
    Q_D(SerialPort);
    return d->setDataErrorPolicy(policy);
}

SerialPort::DataErrorPolicy SerialPort::dataErrorPolicy() const
{
    Q_D(const SerialPort);
    return d->dataErrorPolicy();
}

SerialPort::PortError SerialPort::error() const
{
    Q_D(const SerialPort);
    return d->error();
}

void SerialPort::unsetError()
{
    Q_D(SerialPort);
    d->unsetError();
}

bool SerialPort::isSequential() const
{
    return true;
}

qint64 SerialPort::bytesAvailable() const
{
    Q_D(const SerialPort);
    if (d->m_isBuffered)
        return qint64(d->m_readBuffer.size());
    return d->bytesAvailable();
}

qint64 SerialPort::bytesToWrite() const
{
    Q_D(const SerialPort);
    if (d->m_isBuffered)
        return qint64(d->m_writeBuffer.size());
    return d->bytesToWrite();
}

bool SerialPort::canReadLine() const
{
    Q_D(const SerialPort);
    bool hasLine = d->m_readBuffer.canReadLine();
    return (hasLine || QIODevice::canReadLine());
}

// Returns the difference between msecs and elapsed. If msecs is -1,
// however, -1 is returned.
static int qt_timeout_value(int msecs, int elapsed)
{
    if (msecs == -1) { return msecs; }
    msecs -= elapsed;
    return (msecs < 0) ? 0 : msecs;
}

bool SerialPort::waitForReadyRead(int msecs)
{
    Q_D(SerialPort);

    if (d->m_isBuffered && (!d->m_readBuffer.isEmpty()))
        return true;

    if (d->m_engine->isReadNotificationEnabled())
        d->m_engine->setReadNotificationEnabled(false);

#if QT_VERSION >= 0x040700
    QElapsedTimer stopWatch;
#else
    QTime stopWatch;
#endif

    stopWatch.start();

    forever {
        bool readyToRead = false;
        bool readyToWrite = false;
        if (!d->waitForReadOrWrite(qt_timeout_value(msecs, stopWatch.elapsed()),
                                   true, (!d->m_writeBuffer.isEmpty()),
                                   &readyToRead, &readyToWrite)) {
            return false;
        }
        if (readyToRead) {
            if (d->canReadNotification())
                return true;
        }
        if (readyToWrite)
            d->canWriteNotification();
    }
    return false;
}

bool SerialPort::waitForBytesWritten(int msecs)
{
    Q_D(SerialPort);

    if (d->m_isBuffered && d->m_writeBuffer.isEmpty())
        return false;

#if QT_VERSION >= 0x040700
    QElapsedTimer stopWatch;
#else
    QTime stopWatch;
#endif

    stopWatch.start();

    forever {
        bool readyToRead = false;
        bool readyToWrite = false;
        if (!d->waitForReadOrWrite(qt_timeout_value(msecs, stopWatch.elapsed()),
                                   true, (!d->m_writeBuffer.isEmpty()),
                                   &readyToRead, &readyToWrite)) {
            return false;
        }
        if (readyToRead) {
            if(!d->canReadNotification())
                return false;
        }
        if (readyToWrite) {
            if (d->canWriteNotification()) {
                return true;
            }
        }
    }
    return false;
}

/* Public slots */

bool SerialPort::setDtr(bool set)
{
    Q_D(SerialPort);
    return d->setDtr(set);
}

bool SerialPort::setRts(bool set)
{
    Q_D(SerialPort);
    return d->setRts(set);
}

bool SerialPort::sendBreak(int duration)
{
    Q_D(SerialPort);
    return d->sendBreak(duration);
}

bool SerialPort::setBreak(bool set)
{
    Q_D(SerialPort);
    return d->setBreak(set);
}

/* Protected methods */

qint64 SerialPort::readData(char *data, qint64 maxSize)
{
    Q_D(SerialPort);

    if (d->m_engine->isReadNotificationEnabled() && d->m_isBuffered)
        d->m_engine->setReadNotificationEnabled(true);

    if (!d->m_isBuffered) {
        qint64 readBytes = d->read(data, maxSize);
        return readBytes;
    }

    if (d->m_readBuffer.isEmpty())
        return qint64(0);

    // If readFromSerial() read data, copy it to its destination.
    if (maxSize == 1) {
        *data = d->m_readBuffer.getChar();
        return 1;
    }

    qint64 bytesToRead = qMin(qint64(d->m_readBuffer.size()), maxSize);
    qint64 readSoFar = 0;
    while (readSoFar < bytesToRead) {
        const char *ptr = d->m_readBuffer.readPointer();
        int bytesToReadFromThisBlock = qMin(int(bytesToRead - readSoFar),
                                            d->m_readBuffer.nextDataBlockSize());
        memcpy(data + readSoFar, ptr, bytesToReadFromThisBlock);
        readSoFar += bytesToReadFromThisBlock;
        d->m_readBuffer.free(bytesToReadFromThisBlock);
    }
    return readSoFar;
}

qint64 SerialPort::readLineData(char *data, qint64 maxSize)
{
    return QIODevice::readLineData(data, maxSize);
}

qint64 SerialPort::writeData(const char *data, qint64 maxSize)
{
    Q_D(SerialPort);

    if (!d->m_isBuffered) {
        qint64 written = d->write(data, maxSize);
        if (written < 0) {
            //Error
        } else if (!d->m_writeBuffer.isEmpty()) {
            d->m_engine->setWriteNotificationEnabled(true);
        }

        if (written >= 0)
            emit bytesWritten(written);

        return written;
    }

    char *ptr = d->m_writeBuffer.reserve(maxSize);
    if (maxSize == 1)
        *ptr = *data;
    else
        memcpy(ptr, data, maxSize);

    if (!d->m_writeBuffer.isEmpty())
        d->m_engine->setWriteNotificationEnabled(true);

    return maxSize;
}
