/*
    License...
*/

#include "serialport.h"
#include "serialportinfo.h"
#include "serialport_p.h"

#if QT_VERSION >= 0x040700
#  include <QtCore/QElapsedTimer>
#else
#  include <QtCore/QTime>
#endif

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
                d->setReadNotificationEnabled(true);
            if (mode & WriteOnly)
                d->setWriteNotificationEnabled(true);

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
        d->setReadNotificationEnabled(false);
        d->setWriteNotificationEnabled(false);
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

bool SerialPort::setDataInterval(int usecs)
{
    Q_D(SerialPort);
    return (openMode() & Unbuffered) ? d->setDataInterval(usecs) : false;
}

int SerialPort::dataInterval() const
{
    Q_D(const SerialPort);
    return d->dataInterval();
}

bool SerialPort::setReadTimeout(int msecs)
{
    Q_D(SerialPort);
    return (openMode() & Unbuffered) ? d->setReadTimeout(msecs) : false;
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
    Q_D(SerialPort);
    return d->flush() || d->nativeFlush();
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

    if (!d->m_writeBuffer.isEmpty())
        d->setWriteNotificationEnabled(true);

    return maxSize;
}


