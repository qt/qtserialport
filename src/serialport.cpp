/*
    License...
*/

#include "serialport.h"
#include "serialportinfo.h"
#include "serialport_p.h"

#include <QtCore/QElapsedTimer>

SerialPort::SerialPort(QObject *parent)
    : QIODevice(parent)
    , d_ptr(new SerialPortPrivate())
{
    Q_D(SerialPort);
    d->q_ptr = this;
}

SerialPort::SerialPort(const QString &name, QObject *parent)
    : QIODevice(parent)
    , d_ptr(new SerialPortPrivate())
{
    Q_D(SerialPort);
    d->q_ptr = this;
}

SerialPort::SerialPort(const SerialPortInfo &info, QObject *parent)
    : QIODevice(parent)
    , d_ptr(new SerialPortPrivate())
{
    Q_D(SerialPort);
    d->q_ptr = this;
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

    if ((mode & unsupportedModes) || (mode == NotOpen))
        return false;

    if (!isOpen()) {
        if (d->open(mode)) {
            QIODevice::open(mode);

            d->clearBuffers();

            if (mode & ReadOnly)
                d->setReadNotificationEnabled(true);
            if (mode & WriteOnly)
                d->setWriteNotificationEnabled(true);

            d->m_isBuffered = (0 == (mode & Unbuffered));
            d->setError(SerialPort::NoError);
            return QIODevice::open(mode);
        }
        else
            d->setError(SerialPort::PermissionDeniedError);
    }
    else
        d->setError(SerialPort::DeviceAlreadyOpenedError);

    return false;
}

void SerialPort::close()
{
    Q_D(SerialPort);
    if (isOpen()) {
        d->setReadNotificationEnabled(false, true);
        d->setWriteNotificationEnabled(false, true);
        d->clearBuffers();
        d->close();
        QIODevice::close();
        d->setError(SerialPort::NoError);
    }
    else
        d->setError(SerialPort::DeviceIsNotOpenedError);
}

bool SerialPort::setRate(qint32 rate, Directions dir)
{
    // Impl me
    return false;
}

qint32 SerialPort::rate(Directions dir) const
{
    Q_D(const SerialPort);
    return d->rate(dir);
}

bool SerialPort::setDataBits(DataBits dataBits)
{
    // Impl me
    return false;
}

SerialPort::DataBits SerialPort::dataBits() const
{
    Q_D(const SerialPort);
    return d->dataBits();
}

bool SerialPort::setParity(Parity parity)
{
    // Impl me
    return false;
}

SerialPort::Parity SerialPort::parity() const
{
    Q_D(const SerialPort);
    return d->parity();
}

bool SerialPort::setStopBits(StopBits stopBits)
{
    // Impl me
    return false;
}

SerialPort::StopBits SerialPort::stopBits() const
{
    Q_D(const SerialPort);
    return d->stopBits();
}

bool SerialPort::setFlowControl(FlowControl flow)
{
    // Impl me
    return false;
}

SerialPort::FlowControl SerialPort::flowControl() const
{
    Q_D(const SerialPort);
    return d->flowControl();
}

bool SerialPort::setDataInterval(int usecs)
{
    // Impl me
    return false;
}

int SerialPort::dataInterval() const
{
    Q_D(const SerialPort);
    return d->dataInterval();
}

bool SerialPort::setReadTimeout(int msecs)
{
    // Impl me
    return false;
}

int SerialPort::readTimeout() const
{
    Q_D(const SerialPort);
    return d->readTimeout();
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
    // Impl me
    return false;
}

bool SerialPort::reset()
{
    // Impl me
    return false;
}

bool SerialPort::setDataErrorPolicy(DataErrorPolicy policy)
{
    // Impl me
    return false;
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
    qint64 available = QIODevice::bytesAvailable();
    if (d->m_isBuffered)
        available += qint64(d->m_readBuffer.size());
    else
        available += d->bytesAvailable();
    return available;
}

qint64 SerialPort::bytesToWrite() const
{
    Q_D(const SerialPort);
    return qint64(d->m_writeBuffer.size());
}

/*
   Returns the difference between msecs and elapsed. If msecs is -1,
   however, -1 is returned.
*/
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

    if (d->isReadNotificationEnabled())
        d->setReadNotificationEnabled(false);

    QElapsedTimer stopWatch;
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

    QElapsedTimer stopWatch;
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

bool SerialPort::setDtr(bool set)
{
    // Impl me
    return false;
}

bool SerialPort::setRts(bool set)
{
    // Impl me
    return false;
}

bool SerialPort::sendBreak(int duration)
{
    // Impl me
    return false;
}

bool SerialPort::setBreak(bool set)
{
    // Impl me
    return false;
}

qint64 SerialPort::readData(char *data, qint64 maxSize)
{
    Q_D(SerialPort);

    if (!this->isReadable())
        return -1;

    if (d->isReadNotificationEnabled() && d->m_isBuffered)
        d->setReadNotificationEnabled(true);

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

qint64 SerialPort::writeData(const char *data, qint64 maxSize)
{
    Q_D(SerialPort);

    if (!this->isWritable())
        return -1;

    if (!d->m_isBuffered) {
        qint64 written = d->write(data, maxSize);
        if (written < 0) {
            //Error
        } else if (!d->m_writeBuffer.isEmpty()) {
            d->setWriteNotificationEnabled(true);
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

    qint64 written = maxSize;

    if (!d->m_writeBuffer.isEmpty())
        d->setWriteNotificationEnabled(true);

    return written;
}


