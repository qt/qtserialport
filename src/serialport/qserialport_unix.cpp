/****************************************************************************
**
** Copyright (C) 2012 Denis Shienkov <denis.shienkov@gmail.com>
** Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
** Copyright (C) 2012 Andre Hartmann <aha_1980@gmx.de>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtSerialPort module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qserialport_unix_p.h"

#include <errno.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef Q_OS_MAC
#if defined (MAC_OS_X_VERSION_10_4) && (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4)
#include <IOKit/serial/ioss.h>
#endif
#endif

#include <QtCore/qelapsedtimer.h>
#include <QtCore/qsocketnotifier.h>
#include <QtCore/qmap.h>

QT_BEGIN_NAMESPACE

QString serialPortLockFilePath(const QString &portName)
{
    static const QStringList lockDirectoryPaths = QStringList()
        << QStringLiteral("/var/lock")
        << QStringLiteral("/etc/locks")
        << QStringLiteral("/var/spool/locks")
        << QStringLiteral("/var/spool/uucp")
        << QStringLiteral("/tmp")
#ifdef Q_OS_ANDROID
        << QStringLiteral("/data/local/tmp")
#endif
    ;

    QString lockFilePath;

    foreach (const QString &lockDirectoryPath, lockDirectoryPaths) {
        QFileInfo lockDirectoryInfo(lockDirectoryPath);
        if (lockDirectoryInfo.isReadable() && lockDirectoryInfo.isWritable()) {
            lockFilePath = lockDirectoryPath;
            break;
        }
    }

    if (lockFilePath.isEmpty()) {
        qWarning("The following directories are not readable or writable for detaling with lock files\n");
        foreach (const QString &lockDirectoryPath, lockDirectoryPaths)
            qWarning("\t%s\n", qPrintable(lockDirectoryPath));
        return QString();
    }

    QString replacedPortName = portName;

    lockFilePath.append(QStringLiteral("/LCK.."));
    lockFilePath.append(replacedPortName.replace(QLatin1Char('/'), QLatin1Char('_')));

    return lockFilePath;
}

class ReadNotifier : public QSocketNotifier
{
    Q_OBJECT
public:
    ReadNotifier(QSerialPortPrivate *d, QObject *parent)
        : QSocketNotifier(d->descriptor, QSocketNotifier::Read, parent)
        , dptr(d)
    {}

protected:
    bool event(QEvent *e) Q_DECL_OVERRIDE {
        bool ret = QSocketNotifier::event(e);
        if (ret)
            dptr->readNotification();
        return ret;
    }

private:
    QSerialPortPrivate *dptr;
};

class WriteNotifier : public QSocketNotifier
{
    Q_OBJECT
public:
    WriteNotifier(QSerialPortPrivate *d, QObject *parent)
        : QSocketNotifier(d->descriptor, QSocketNotifier::Write, parent)
        , dptr(d)
    {}

protected:
    bool event(QEvent *e) Q_DECL_OVERRIDE {
        bool ret = QSocketNotifier::event(e);
        if (ret)
            dptr->writeNotification();
        return ret;
    }

private:
    QSerialPortPrivate *dptr;
};

class ExceptionNotifier : public QSocketNotifier
{
    Q_OBJECT
public:
    ExceptionNotifier(QSerialPortPrivate *d, QObject *parent)
        : QSocketNotifier(d->descriptor, QSocketNotifier::Exception, parent)
        , dptr(d)
    {}

protected:
    bool event(QEvent *e) Q_DECL_OVERRIDE {
        bool ret = QSocketNotifier::event(e);
        if (ret)
            dptr->exceptionNotification();
        return ret;
    }

private:
    QSerialPortPrivate *dptr;
};

#include "qserialport_unix.moc"

QSerialPortPrivate::QSerialPortPrivate(QSerialPort *q)
    : QSerialPortPrivateData(q)
    , descriptor(-1)
    , isCustomBaudRateSupported(false)
    , readNotifier(0)
    , writeNotifier(0)
    , exceptionNotifier(0)
    , readPortNotifierCalled(false)
    , readPortNotifierState(false)
    , readPortNotifierStateSet(false)
    , emittedReadyRead(false)
    , emittedBytesWritten(false)
{
}

bool QSerialPortPrivate::open(QIODevice::OpenMode mode)
{
    Q_Q(QSerialPort);

    QString lockFilePath = serialPortLockFilePath(portNameFromSystemLocation(systemLocation));
    bool isLockFileEmpty = lockFilePath.isEmpty();
    if (isLockFileEmpty) {
        qWarning("Failed to create a lock file for opening the device");
        q->setError(QSerialPort::PermissionError);
        return false;
    }

    QScopedPointer<QLockFile> newLockFileScopedPointer(new QLockFile(lockFilePath));
    lockFileScopedPointer.swap(newLockFileScopedPointer);

    if (lockFileScopedPointer->isLocked()) {
        q->setError(QSerialPort::PermissionError);
        return false;
    }

    int flags = O_NOCTTY | O_NONBLOCK;

    switch (mode & QIODevice::ReadWrite) {
    case QIODevice::WriteOnly:
        flags |= O_WRONLY;
        break;
    case QIODevice::ReadWrite:
        flags |= O_RDWR;
        break;
    default:
        flags |= O_RDONLY;
        break;
    }

    descriptor = ::open(systemLocation.toLocal8Bit().constData(), flags);

    if (descriptor == -1) {
        q->setError(decodeSystemError());
        return false;
    }

    lockFileScopedPointer->lock();
    if (!lockFileScopedPointer->isLocked()) {
        q->setError(QSerialPort::PermissionError);
        return false;
    }

#ifdef TIOCEXCL
    ::ioctl(descriptor, TIOCEXCL);
#endif

    if (::tcgetattr(descriptor, &restoredTermios) == -1) {
        q->setError(decodeSystemError());
        return false;
    }

    currentTermios = restoredTermios;
    ::cfmakeraw(&currentTermios);
    currentTermios.c_cflag |= CLOCAL;
    currentTermios.c_cc[VTIME] = 0;
    currentTermios.c_cc[VMIN] = 0;

    if (mode & QIODevice::ReadOnly)
        currentTermios.c_cflag |= CREAD;

    if (!updateTermios())
        return false;

    setExceptionNotificationEnabled(true);

    if ((flags & O_WRONLY) == 0)
        setReadNotificationEnabled(true);

    detectDefaultSettings();
    return true;
}

void QSerialPortPrivate::close()
{
    if (settingsRestoredOnClose) {
        ::tcsetattr(descriptor, TCSANOW, &restoredTermios);
#ifdef Q_OS_LINUX
        if (isCustomBaudRateSupported)
            ::ioctl(descriptor, TIOCSSERIAL, &restoredSerialInfo);
#endif
    }

#ifdef TIOCNXCL
    ::ioctl(descriptor, TIOCNXCL);
#endif

    if (readNotifier) {
        readNotifier->setEnabled(false);
        readNotifier->deleteLater();
        readNotifier = 0;
    }

    if (writeNotifier) {
        writeNotifier->setEnabled(false);
        writeNotifier->deleteLater();
        writeNotifier = 0;
    }

    if (exceptionNotifier) {
        exceptionNotifier->setEnabled(false);
        exceptionNotifier->deleteLater();
        exceptionNotifier = 0;
    }

    ::close(descriptor);

    if (lockFileScopedPointer->isLocked())
        lockFileScopedPointer->unlock();

    descriptor = -1;
    isCustomBaudRateSupported = false;
}

QSerialPort::PinoutSignals QSerialPortPrivate::pinoutSignals()
{
    Q_Q(QSerialPort);

    int arg = 0;

    if (::ioctl(descriptor, TIOCMGET, &arg) == -1) {
        q->setError(decodeSystemError());
        return QSerialPort::NoSignal;
    }

    QSerialPort::PinoutSignals ret = QSerialPort::NoSignal;

#ifdef TIOCM_LE
    if (arg & TIOCM_LE)
        ret |= QSerialPort::DataSetReadySignal;
#endif
#ifdef TIOCM_DTR
    if (arg & TIOCM_DTR)
        ret |= QSerialPort::DataTerminalReadySignal;
#endif
#ifdef TIOCM_RTS
    if (arg & TIOCM_RTS)
        ret |= QSerialPort::RequestToSendSignal;
#endif
#ifdef TIOCM_ST
    if (arg & TIOCM_ST)
        ret |= QSerialPort::SecondaryTransmittedDataSignal;
#endif
#ifdef TIOCM_SR
    if (arg & TIOCM_SR)
        ret |= QSerialPort::SecondaryReceivedDataSignal;
#endif
#ifdef TIOCM_CTS
    if (arg & TIOCM_CTS)
        ret |= QSerialPort::ClearToSendSignal;
#endif
#ifdef TIOCM_CAR
    if (arg & TIOCM_CAR)
        ret |= QSerialPort::DataCarrierDetectSignal;
#elif defined TIOCM_CD
    if (arg & TIOCM_CD)
        ret |= QSerialPort::DataCarrierDetectSignal;
#endif
#ifdef TIOCM_RNG
    if (arg & TIOCM_RNG)
        ret |= QSerialPort::RingIndicatorSignal;
#elif defined TIOCM_RI
    if (arg & TIOCM_RI)
        ret |= QSerialPort::RingIndicatorSignal;
#endif
#ifdef TIOCM_DSR
    if (arg & TIOCM_DSR)
        ret |= QSerialPort::DataSetReadySignal;
#endif

    return ret;
}

bool QSerialPortPrivate::setDataTerminalReady(bool set)
{
    int status = TIOCM_DTR;
    return ::ioctl(descriptor, set ? TIOCMBIS : TIOCMBIC, &status) != -1;
}

bool QSerialPortPrivate::setRequestToSend(bool set)
{
    int status = TIOCM_RTS;
    return ::ioctl(descriptor, set ? TIOCMBIS : TIOCMBIC, &status) != -1;
}

bool QSerialPortPrivate::flush()
{
    return writeNotification()
#ifndef Q_OS_ANDROID
            && (::tcdrain(descriptor) != -1);
#else
            && (::ioctl(descriptor, TCSBRK, 1) != -1);
#endif
}

bool QSerialPortPrivate::clear(QSerialPort::Directions directions)
{
    return ::tcflush(descriptor, (directions == QSerialPort::AllDirections)
                     ? TCIOFLUSH : (directions & QSerialPort::Input) ? TCIFLUSH : TCOFLUSH) != -1;
}

bool QSerialPortPrivate::sendBreak(int duration)
{
    return ::tcsendbreak(descriptor, duration) != -1;
}

bool QSerialPortPrivate::setBreakEnabled(bool set)
{
    return ::ioctl(descriptor, set ? TIOCSBRK : TIOCCBRK) != -1;
}

qint64 QSerialPortPrivate::systemInputQueueSize () const
{
    int nbytes = 0;
#ifdef TIOCINQ
    if (::ioctl(descriptor, TIOCINQ, &nbytes) == -1)
        return -1;
#endif
    return nbytes;
}

qint64 QSerialPortPrivate::systemOutputQueueSize () const
{
    int nbytes = 0;
#ifdef TIOCOUTQ
    if (::ioctl(descriptor, TIOCOUTQ, &nbytes) == -1)
        return -1;
#endif
    return nbytes;
}

qint64 QSerialPortPrivate::writeToBuffer(const char *data, qint64 maxSize)
{
    char *ptr = writeBuffer.reserve(maxSize);
    if (maxSize == 1)
        *ptr = *data;
    else
        ::memcpy(ptr, data, maxSize);

    const qint64 written = maxSize;

    if (!writeBuffer.isEmpty() && !isWriteNotificationEnabled())
        setWriteNotificationEnabled(true);

    return written;
}

bool QSerialPortPrivate::waitForReadyRead(int msecs)
{
    Q_Q(QSerialPort);

    QElapsedTimer stopWatch;

    stopWatch.start();

    do {
        bool readyToRead = false;
        bool readyToWrite = false;
        bool timedOut = false;
        if (!waitForReadOrWrite(&readyToRead, &readyToWrite, true, !writeBuffer.isEmpty(),
                                timeoutValue(msecs, stopWatch.elapsed()), &timedOut)) {
            if (!timedOut)
                q->setError(decodeSystemError());
            return false;
        }

        if (readyToRead) {
            if (readNotification())
                return true;
        }

        if (readyToWrite)
            writeNotification();

    } while (msecs == -1 || timeoutValue(msecs, stopWatch.elapsed()) > 0);
    return false;
}

bool QSerialPortPrivate::waitForBytesWritten(int msecs)
{
    Q_Q(QSerialPort);

    if (writeBuffer.isEmpty())
        return false;

    QElapsedTimer stopWatch;

    stopWatch.start();

    forever {
        bool readyToRead = false;
        bool readyToWrite = false;
        bool timedOut = false;
        if (!waitForReadOrWrite(&readyToRead, &readyToWrite, true, !writeBuffer.isEmpty(),
                                timeoutValue(msecs, stopWatch.elapsed()), &timedOut)) {
            if (!timedOut)
                q->setError(decodeSystemError());
            return false;
        }

        if (readyToRead && !readNotification())
            return false;

        if (readyToWrite)
            return writeNotification();
    }
    return false;
}

bool QSerialPortPrivate::setBaudRate(qint32 baudRate, QSerialPort::Directions directions)
{
    Q_Q(QSerialPort);

    bool ret = baudRate > 0;

    // prepare section

    if (ret) {
        const qint32 unixBaudRate = QSerialPortPrivate::settingFromBaudRate(baudRate);
        if (unixBaudRate > 0) {
            // try prepate to set standard baud rate
#ifdef Q_OS_LINUX
            // prepare to forcefully reset the custom mode
            if (isCustomBaudRateSupported) {
                //currentSerialInfo.flags |= ASYNC_SPD_MASK;
                currentSerialInfo.flags &= ~(ASYNC_SPD_CUST /* | ASYNC_LOW_LATENCY*/);
                currentSerialInfo.custom_divisor = 0;
            }
#endif
            // prepare to set standard baud rate
            ret = !(((directions & QSerialPort::Input) && ::cfsetispeed(&currentTermios, unixBaudRate) < 0)
                    || ((directions & QSerialPort::Output) && ::cfsetospeed(&currentTermios, unixBaudRate) < 0));
        } else {
            // try prepate to set custom baud rate
#ifdef Q_OS_LINUX
            // prepare to forcefully set the custom mode
            if (isCustomBaudRateSupported) {
                currentSerialInfo.flags &= ~ASYNC_SPD_MASK;
                currentSerialInfo.flags |= (ASYNC_SPD_CUST /* | ASYNC_LOW_LATENCY*/);
                currentSerialInfo.custom_divisor = currentSerialInfo.baud_base / baudRate;
                if (currentSerialInfo.custom_divisor == 0)
                    currentSerialInfo.custom_divisor = 1;
                // for custom mode needed prepare to set B38400 baud rate
                ret = (::cfsetispeed(&currentTermios, B38400) != -1) && (::cfsetospeed(&currentTermios, B38400) != -1);
            } else {
                ret = false;
            }
#elif defined(Q_OS_MAC)

#  if defined (MAC_OS_X_VERSION_10_4) && (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4)
            // Starting with Tiger, the IOSSIOSPEED ioctl can be used to set arbitrary baud rates
            // other than those specified by POSIX. The driver for the underlying serial hardware
            // ultimately determines which baud rates can be used. This ioctl sets both the input
            // and output speed.
            ret = ::ioctl(descriptor, IOSSIOSPEED, &baudRate) != -1;
#  else
            // others MacOSX version, can't prepare to set custom baud rate
            ret = false;
#  endif

#else
            // others *nix OS, can't prepare to set custom baud rate
            ret = false;
#endif
        }
    }

    // finally section

#ifdef Q_OS_LINUX
    if (ret && isCustomBaudRateSupported) // finally, set or reset the custom mode
        ret = ::ioctl(descriptor, TIOCSSERIAL, &currentSerialInfo) != -1;
#endif

    if (ret) // finally, set baud rate
        ret = updateTermios();
    else
        q->setError(decodeSystemError());
    return ret;
}

bool QSerialPortPrivate::setDataBits(QSerialPort::DataBits dataBits)
{
    currentTermios.c_cflag &= ~CSIZE;
    switch (dataBits) {
    case QSerialPort::Data5:
        currentTermios.c_cflag |= CS5;
        break;
    case QSerialPort::Data6:
        currentTermios.c_cflag |= CS6;
        break;
    case QSerialPort::Data7:
        currentTermios.c_cflag |= CS7;
        break;
    case QSerialPort::Data8:
        currentTermios.c_cflag |= CS8;
        break;
    default:
        currentTermios.c_cflag |= CS8;
        break;
    }
    return updateTermios();
}

bool QSerialPortPrivate::setParity(QSerialPort::Parity parity)
{
    currentTermios.c_iflag &= ~(PARMRK | INPCK);
    currentTermios.c_iflag |= IGNPAR;

    switch (parity) {

#ifdef CMSPAR
    // Here Installation parity only for GNU/Linux where the macro CMSPAR.
    case QSerialPort::SpaceParity:
        currentTermios.c_cflag &= ~PARODD;
        currentTermios.c_cflag |= PARENB | CMSPAR;
        break;
    case QSerialPort::MarkParity:
        currentTermios.c_cflag |= PARENB | CMSPAR | PARODD;
        break;
#endif
    case QSerialPort::NoParity:
        currentTermios.c_cflag &= ~PARENB;
        break;
    case QSerialPort::EvenParity:
        currentTermios.c_cflag &= ~PARODD;
        currentTermios.c_cflag |= PARENB;
        break;
    case QSerialPort::OddParity:
        currentTermios.c_cflag |= PARENB | PARODD;
        break;
    default:
        currentTermios.c_cflag |= PARENB;
        currentTermios.c_iflag |= PARMRK | INPCK;
        currentTermios.c_iflag &= ~IGNPAR;
        break;
    }

    return updateTermios();
}

bool QSerialPortPrivate::setStopBits(QSerialPort::StopBits stopBits)
{
    switch (stopBits) {
    case QSerialPort::OneStop:
        currentTermios.c_cflag &= ~CSTOPB;
        break;
    case QSerialPort::TwoStop:
        currentTermios.c_cflag |= CSTOPB;
        break;
    default:
        currentTermios.c_cflag &= ~CSTOPB;
        break;
    }
    return updateTermios();
}

bool QSerialPortPrivate::setFlowControl(QSerialPort::FlowControl flow)
{
    switch (flow) {
    case QSerialPort::NoFlowControl:
        currentTermios.c_cflag &= ~CRTSCTS;
        currentTermios.c_iflag &= ~(IXON | IXOFF | IXANY);
        break;
    case QSerialPort::HardwareControl:
        currentTermios.c_cflag |= CRTSCTS;
        currentTermios.c_iflag &= ~(IXON | IXOFF | IXANY);
        break;
    case QSerialPort::SoftwareControl:
        currentTermios.c_cflag &= ~CRTSCTS;
        currentTermios.c_iflag |= IXON | IXOFF | IXANY;
        break;
    default:
        currentTermios.c_cflag &= ~CRTSCTS;
        currentTermios.c_iflag &= ~(IXON | IXOFF | IXANY);
        break;
    }
    return updateTermios();
}

bool QSerialPortPrivate::setDataErrorPolicy(QSerialPort::DataErrorPolicy policy)
{
    tcflag_t parmrkMask = PARMRK;
#ifndef CMSPAR
    // in space/mark parity emulation also used PARMRK flag
    if (parity == QSerialPort::SpaceParity
            || parity == QSerialPort::MarkParity) {
        parmrkMask = 0;
    }
#endif //CMSPAR
    switch (policy) {
    case QSerialPort::SkipPolicy:
        currentTermios.c_iflag &= ~parmrkMask;
        currentTermios.c_iflag |= IGNPAR | INPCK;
        break;
    case QSerialPort::PassZeroPolicy:
        currentTermios.c_iflag &= ~(IGNPAR | parmrkMask);
        currentTermios.c_iflag |= INPCK;
        break;
    case QSerialPort::IgnorePolicy:
        currentTermios.c_iflag &= ~INPCK;
        break;
    case QSerialPort::StopReceivingPolicy:
        currentTermios.c_iflag &= ~IGNPAR;
        currentTermios.c_iflag |= parmrkMask | INPCK;
        break;
    default:
        currentTermios.c_iflag &= ~INPCK;
        break;
    }
    return updateTermios();
}

bool QSerialPortPrivate::readNotification()
{
    Q_Q(QSerialPort);

    // Prevent recursive calls
    if (readPortNotifierCalled) {
        if (!readPortNotifierStateSet) {
            readPortNotifierStateSet = true;
            readPortNotifierState = isReadNotificationEnabled();
            setReadNotificationEnabled(false);
        }
    }

    readPortNotifierCalled = true;

    // Always buffered, read data from the port into the read buffer
    qint64 newBytes = readBuffer.size();
    qint64 bytesToRead = policy == QSerialPort::IgnorePolicy ? ReadChunkSize : 1;

    if (readBufferMaxSize && bytesToRead > (readBufferMaxSize - readBuffer.size())) {
        bytesToRead = readBufferMaxSize - readBuffer.size();
        if (bytesToRead == 0) {
            // Buffer is full. User must read data from the buffer
            // before we can read more from the port.
            return false;
        }
    }

    char *ptr = readBuffer.reserve(bytesToRead);
    const qint64 readBytes = readFromPort(ptr, bytesToRead);

    if (readBytes <= 0) {
        QSerialPort::SerialPortError error = decodeSystemError();
        if (error != QSerialPort::ResourceError)
            error = QSerialPort::ReadError;
        q->setError(error);
        readBuffer.chop(bytesToRead);
        return false;
    }

    readBuffer.chop(bytesToRead - qMax(readBytes, qint64(0)));

    newBytes = readBuffer.size() - newBytes;

    // If read buffer is full, disable the read port notifier.
    if (readBufferMaxSize && readBuffer.size() == readBufferMaxSize)
        setReadNotificationEnabled(false);

    // only emit readyRead() when not recursing, and only if there is data available
    const bool hasData = newBytes > 0;

    if (!emittedReadyRead && hasData) {
        emittedReadyRead = true;
        emit q->readyRead();
        emittedReadyRead = false;
    }

    if (!hasData)
        setReadNotificationEnabled(true);

    // reset the read port notifier state if we reentered inside the
    // readyRead() connected slot.
    if (readPortNotifierStateSet
            && readPortNotifierState != isReadNotificationEnabled()) {
        setReadNotificationEnabled(readPortNotifierState);
        readPortNotifierStateSet = false;
    }
    return true;
}

bool QSerialPortPrivate::writeNotification()
{
    Q_Q(QSerialPort);

    const int tmp = writeBuffer.size();

    if (writeBuffer.isEmpty()) {
        setWriteNotificationEnabled(false);
        return false;
    }

    int nextSize = writeBuffer.nextDataBlockSize();

    const char *ptr = writeBuffer.readPointer();

    // Attempt to write it all in one chunk.
    qint64 written = writeToPort(ptr, nextSize);
    if (written < 0) {
        QSerialPort::SerialPortError error = decodeSystemError();
        if (error != QSerialPort::ResourceError)
            error = QSerialPort::WriteError;
        q->setError(error);
        return false;
    }

    // Remove what we wrote so far.
    writeBuffer.free(written);
    if (written > 0) {
        // Don't emit bytesWritten() recursively.
        if (!emittedBytesWritten) {
            emittedBytesWritten = true;
            emit q->bytesWritten(written);
            emittedBytesWritten = false;
        }
    }

    if (writeBuffer.isEmpty())
        setWriteNotificationEnabled(false);

    return (writeBuffer.size() < tmp);
}

void QSerialPortPrivate::exceptionNotification()
{
    Q_Q(QSerialPort);

    QSerialPort::SerialPortError error = decodeSystemError();
    q->setError(error);
}

bool QSerialPortPrivate::updateTermios()
{
    Q_Q(QSerialPort);

    if (::tcsetattr(descriptor, TCSANOW, &currentTermios) == -1) {
        q->setError(decodeSystemError());
        return false;
    }
    return true;
}

void QSerialPortPrivate::detectDefaultSettings()
{
    // Detect baud rate.
    const speed_t inputUnixBaudRate = ::cfgetispeed(&currentTermios);
    const speed_t outputUnixBaudRate = ::cfgetospeed(&currentTermios);
    bool isCustomBaudRateCurrentSet = false;

#ifdef Q_OS_LINUX
    // try detect the ability to support custom baud rate
    isCustomBaudRateSupported = ::ioctl(descriptor, TIOCGSERIAL, &currentSerialInfo) != -1
            && ::ioctl(descriptor, TIOCSSERIAL, &currentSerialInfo) != -1;

    if (isCustomBaudRateSupported) {
        restoredSerialInfo = currentSerialInfo;

        // assume that the baud rate is a custom
        isCustomBaudRateCurrentSet = inputUnixBaudRate == B38400 && outputUnixBaudRate == B38400;

        if (isCustomBaudRateCurrentSet) {
            if ((currentSerialInfo.flags & ASYNC_SPD_CUST)
                    && currentSerialInfo.custom_divisor > 0) {

                // yes, speed is really custom
                inputBaudRate = currentSerialInfo.baud_base / currentSerialInfo.custom_divisor;
                outputBaudRate = inputBaudRate;
            } else {
                // no, we were wrong and the speed is a standard 38400 baud
                isCustomBaudRateCurrentSet = false;
            }
        }
    }
#else
    // other *nix
#endif
    if (!isCustomBaudRateSupported || !isCustomBaudRateCurrentSet) {
        inputBaudRate = QSerialPortPrivate::baudRateFromSetting(inputUnixBaudRate);
        outputBaudRate = QSerialPortPrivate::baudRateFromSetting(outputUnixBaudRate);
    }

    // Detect databits.
    switch (currentTermios.c_cflag & CSIZE) {
    case CS5:
        dataBits = QSerialPort::Data5;
        break;
    case CS6:
        dataBits = QSerialPort::Data6;
        break;
    case CS7:
        dataBits = QSerialPort::Data7;
        break;
    case CS8:
        dataBits = QSerialPort::Data8;
        break;
    default:
        qWarning("%s: Unexpected data bits settings", Q_FUNC_INFO);
        dataBits = QSerialPort::Data8;
        break;
    }

    // Detect parity.
#ifdef CMSPAR
    if (currentTermios.c_cflag & CMSPAR) {
        parity = currentTermios.c_cflag & PARODD ?
                    QSerialPort::MarkParity : QSerialPort::SpaceParity;
    } else {
#endif
        if (currentTermios.c_cflag & PARENB) {
            parity = currentTermios.c_cflag & PARODD ?
                        QSerialPort::OddParity : QSerialPort::EvenParity;
        } else {
            parity = QSerialPort::NoParity;
        }
#ifdef CMSPAR
    }
#endif

    // Detect stopbits.
    stopBits = currentTermios.c_cflag & CSTOPB ?
                QSerialPort::TwoStop : QSerialPort::OneStop;

    // Detect flow control.
    if ((!(currentTermios.c_cflag & CRTSCTS)) && (!(currentTermios.c_iflag & (IXON | IXOFF | IXANY))))
        flow = QSerialPort::NoFlowControl;
    else if ((!(currentTermios.c_cflag & CRTSCTS)) && (currentTermios.c_iflag & (IXON | IXOFF | IXANY)))
        flow = QSerialPort::SoftwareControl;
    else if ((currentTermios.c_cflag & CRTSCTS) && (!(currentTermios.c_iflag & (IXON | IXOFF | IXANY))))
        flow = QSerialPort::HardwareControl;
    else {
        qWarning("%s: Unexpected flow control settings", Q_FUNC_INFO);
        flow = QSerialPort::NoFlowControl;
    }
}

QSerialPort::SerialPortError QSerialPortPrivate::decodeSystemError() const
{
    QSerialPort::SerialPortError error;
    switch (errno) {
    case ENODEV:
        error = QSerialPort::DeviceNotFoundError;
        break;
    case EACCES:
        error = QSerialPort::PermissionError;
        break;
    case EBUSY:
        error = QSerialPort::PermissionError;
        break;
    case EAGAIN:
        error = QSerialPort::ResourceError;
        break;
    case EIO:
        error = QSerialPort::ResourceError;
        break;
    case EBADF:
        error = QSerialPort::ResourceError;
        break;
#ifdef Q_OS_MAC
    case ENXIO:
        error = QSerialPort::ResourceError;
        break;
#endif
    default:
        error = QSerialPort::UnknownError;
        break;
    }
    return error;
}

bool QSerialPortPrivate::isReadNotificationEnabled() const
{
    return readNotifier && readNotifier->isEnabled();
}

void QSerialPortPrivate::setReadNotificationEnabled(bool enable)
{
    Q_Q(QSerialPort);

    if (readNotifier) {
        readNotifier->setEnabled(enable);
    } else if (enable) {
        readNotifier = new ReadNotifier(this, q);
        readNotifier->setEnabled(true);
    }
}

bool QSerialPortPrivate::isWriteNotificationEnabled() const
{
    return writeNotifier && writeNotifier->isEnabled();
}

void QSerialPortPrivate::setWriteNotificationEnabled(bool enable)
{
    Q_Q(QSerialPort);

    if (writeNotifier) {
        writeNotifier->setEnabled(enable);
    } else if (enable) {
        writeNotifier = new WriteNotifier(this, q);
        writeNotifier->setEnabled(true);
    }
}

bool QSerialPortPrivate::isExceptionNotificationEnabled() const
{
    return exceptionNotifier && exceptionNotifier->isEnabled();
}

void QSerialPortPrivate::setExceptionNotificationEnabled(bool enable)
{
    Q_Q(QSerialPort);

    if (exceptionNotifier) {
        exceptionNotifier->setEnabled(enable);
    } else if (enable) {
        exceptionNotifier = new ExceptionNotifier(this, q);
        exceptionNotifier->setEnabled(true);
    }
}

bool QSerialPortPrivate::waitForReadOrWrite(bool *selectForRead, bool *selectForWrite,
                                           bool checkRead, bool checkWrite,
                                           int msecs, bool *timedOut)
{
    Q_Q(QSerialPort);

    Q_ASSERT(selectForRead);
    Q_ASSERT(selectForWrite);
    Q_ASSERT(timedOut);

    fd_set fdread;
    FD_ZERO(&fdread);
    if (checkRead)
        FD_SET(descriptor, &fdread);

    fd_set fdwrite;
    FD_ZERO(&fdwrite);
    if (checkWrite)
        FD_SET(descriptor, &fdwrite);

    struct timeval tv;
    tv.tv_sec = msecs / 1000;
    tv.tv_usec = (msecs % 1000) * 1000;

    int ret = ::select(descriptor + 1, &fdread, &fdwrite, 0, msecs < 0 ? 0 : &tv);
    if (ret < 0)
        return false;
    if (ret == 0) {
        *timedOut = true;
        q->setError(QSerialPort::TimeoutError);
        return false;
    }

    *selectForRead = FD_ISSET(descriptor, &fdread);
    *selectForWrite = FD_ISSET(descriptor, &fdwrite);

    return ret;
}

qint64 QSerialPortPrivate::readFromPort(char *data, qint64 maxSize)
{
    qint64 bytesRead = 0;
#if defined (CMSPAR)
    if (parity == QSerialPort::NoParity
            || policy != QSerialPort::StopReceivingPolicy) {
#else
    if (parity != QSerialPort::MarkParity
            && parity != QSerialPort::SpaceParity) {
#endif
        bytesRead = ::read(descriptor, data, maxSize);
    } else {// Perform parity emulation.
        bytesRead = readPerChar(data, maxSize);
    }

    return bytesRead;
}

qint64 QSerialPortPrivate::writeToPort(const char *data, qint64 maxSize)
{
    qint64 bytesWritten = 0;
#if defined (CMSPAR)
    bytesWritten = ::write(descriptor, data, maxSize);
#else
    if (parity != QSerialPort::MarkParity
            && parity != QSerialPort::SpaceParity) {
        bytesWritten = ::write(descriptor, data, maxSize);
    } else {// Perform parity emulation.
        bytesWritten = writePerChar(data, maxSize);
    }
#endif

    return bytesWritten;
}

static inline bool evenParity(quint8 c)
{
    c ^= c >> 4;        //(c7 ^ c3)(c6 ^ c2)(c5 ^ c1)(c4 ^ c0)
    c ^= c >> 2;        //[(c7 ^ c3)(c5 ^ c1)][(c6 ^ c2)(c4 ^ c0)]
    c ^= c >> 1;
    return c & 1;       //(c7 ^ c3)(c5 ^ c1)(c6 ^ c2)(c4 ^ c0)
}

#ifndef CMSPAR

qint64 QSerialPortPrivate::writePerChar(const char *data, qint64 maxSize)
{
    qint64 ret = 0;
    quint8 const charMask = (0xFF >> (8 - dataBits));

    while (ret < maxSize) {

        bool par = evenParity(*data & charMask);
        // False if need EVEN, true if need ODD.
        par ^= parity == QSerialPort::MarkParity;
        if (par ^ (currentTermios.c_cflag & PARODD)) { // Need switch parity mode?
            currentTermios.c_cflag ^= PARODD;
            flush(); //force sending already buffered data, because updateTermios() cleares buffers
            //todo: add receiving buffered data!!!
            if (!updateTermios())
                break;
        }

        int r = ::write(descriptor, data, 1);
        if (r < 0)
            return -1;
        if (r > 0) {
            data += r;
            ret += r;
        }
    }
    return ret;
}

#endif //CMSPAR

qint64 QSerialPortPrivate::readPerChar(char *data, qint64 maxSize)
{
    Q_Q(QSerialPort);

    qint64 ret = 0;
    quint8 const charMask = (0xFF >> (8 - dataBits));

    // 0 - prefix not started,
    // 1 - received 0xFF,
    // 2 - received 0xFF and 0x00
    int prefix = 0;
    while (ret < maxSize) {

        qint64 r = ::read(descriptor, data, 1);
        if (r < 0) {
            if (errno == EAGAIN) // It is ok for nonblocking mode.
                break;
            return -1;
        }
        if (r == 0)
            break;

        bool par = true;
        switch (prefix) {
        case 2: // Previously received both 0377 and 0.
            par = false;
            prefix = 0;
            break;
        case 1: // Previously received 0377.
            if (*data == '\0') {
                ++prefix;
                continue;
            }
            prefix = 0;
            break;
        default:
            if (*data == '\377') {
                prefix = 1;
                continue;
            }
            break;
        }
        // Now: par contains parity ok or error, *data contains received character
        par ^= evenParity(*data & charMask); //par contains parity bit value for EVEN mode
        par ^= (currentTermios.c_cflag & PARODD); //par contains parity bit value for current mode
        if (par ^ (parity == QSerialPort::SpaceParity)) { //if parity error
            switch (policy) {
            case QSerialPort::SkipPolicy:
                continue;       //ignore received character
            case QSerialPort::StopReceivingPolicy:
                if (parity != QSerialPort::NoParity)
                    q->setError(QSerialPort::ParityError);
                else
                    q->setError(*data == '\0' ?
                                QSerialPort::BreakConditionError : QSerialPort::FramingError);
                return ++ret;   //abort receiving
                break;
            case QSerialPort::UnknownPolicy:
                // Unknown error policy is used! Falling back to PassZeroPolicy
            case QSerialPort::PassZeroPolicy:
                *data = '\0';   //replace received character by zero
                break;
            case QSerialPort::IgnorePolicy:
                break;          //ignore error and pass received character
            }
        }
        ++data;
        ++ret;
    }
    return ret;
}

#ifdef Q_OS_MAC
static const QString defaultFilePathPrefix = QStringLiteral("/dev/cu.");
static const QString unusedFilePathPrefix = QStringLiteral("/dev/tty.");
#else
static const QString defaultFilePathPrefix = QStringLiteral("/dev/");
#endif

QString QSerialPortPrivate::portNameToSystemLocation(const QString &port)
{
    QString ret = port;

#ifdef Q_OS_MAC
    ret.remove(unusedFilePathPrefix);
#endif

    if (!ret.contains(defaultFilePathPrefix))
        ret.prepend(defaultFilePathPrefix);
    return ret;
}

QString QSerialPortPrivate::portNameFromSystemLocation(const QString &location)
{
    QString ret = location;

#ifdef Q_OS_MAC
    ret.remove(unusedFilePathPrefix);
#endif

    ret.remove(defaultFilePathPrefix);
    return ret;
}

typedef QMap<qint32, qint32> BaudRateMap;

// The OS specific defines can be found in termios.h

static const BaudRateMap createStandardBaudRateMap()
{
    BaudRateMap baudRateMap;

#ifdef B50
    baudRateMap.insert(50, B50);
#endif

#ifdef B75
    baudRateMap.insert(75, B75);
#endif

#ifdef B110
    baudRateMap.insert(110, B110);
#endif

#ifdef B134
    baudRateMap.insert(134, B134);
#endif

#ifdef B150
    baudRateMap.insert(150, B150);
#endif

#ifdef B200
    baudRateMap.insert(200, B200);
#endif

#ifdef B300
    baudRateMap.insert(300, B300);
#endif

#ifdef B600
    baudRateMap.insert(600, B600);
#endif

#ifdef B1200
    baudRateMap.insert(1200, B1200);
#endif

#ifdef B1800
    baudRateMap.insert(1800, B1800);
#endif

#ifdef B2400
    baudRateMap.insert(2400, B2400);
#endif

#ifdef B4800
    baudRateMap.insert(4800, B4800);
#endif

#ifdef B7200
    baudRateMap.insert(7200, B7200);
#endif

#ifdef B9600
    baudRateMap.insert(9600, B9600);
#endif

#ifdef B14400
    baudRateMap.insert(14400, B14400);
#endif

#ifdef B19200
    baudRateMap.insert(19200, B19200);
#endif

#ifdef B28800
    baudRateMap.insert(28800, B28800);
#endif

#ifdef B38400
    baudRateMap.insert(38400, B38400);
#endif

#ifdef B57600
    baudRateMap.insert(57600, B57600);
#endif

#ifdef B76800
    baudRateMap.insert(76800, B76800);
#endif

#ifdef B115200
    baudRateMap.insert(115200, B115200);
#endif

#ifdef B230400
    baudRateMap.insert(230400, B230400);
#endif

#ifdef B460800
    baudRateMap.insert(460800, B460800);
#endif

#ifdef B500000
    baudRateMap.insert(500000, B500000);
#endif

#ifdef B576000
    baudRateMap.insert(576000, B576000);
#endif

#ifdef B921600
    baudRateMap.insert(921600, B921600);
#endif

#ifdef B1000000
    baudRateMap.insert(1000000, B1000000);
#endif

#ifdef B1152000
    baudRateMap.insert(1152000, B1152000);
#endif

#ifdef B1500000
    baudRateMap.insert(1500000, B1500000);
#endif

#ifdef B2000000
    baudRateMap.insert(2000000, B2000000);
#endif

#ifdef B2500000
    baudRateMap.insert(2500000, B2500000);
#endif

#ifdef B3000000
    baudRateMap.insert(3000000, B3000000);
#endif

#ifdef B3500000
    baudRateMap.insert(3500000, B3500000);
#endif

#ifdef B4000000
    baudRateMap.insert(4000000, B4000000);
#endif

    return baudRateMap;
}

static const BaudRateMap& standardBaudRateMap()
{
    static const BaudRateMap baudRateMap = createStandardBaudRateMap();
    return baudRateMap;
}

qint32 QSerialPortPrivate::baudRateFromSetting(qint32 setting)
{
    return standardBaudRateMap().key(setting);
}

qint32 QSerialPortPrivate::settingFromBaudRate(qint32 baudRate)
{
    return standardBaudRateMap().value(baudRate);
}

QList<qint32> QSerialPortPrivate::standardBaudRates()
{
    return standardBaudRateMap().keys();
}

QSerialPort::Handle QSerialPort::handle() const
{
    Q_D(const QSerialPort);
    return d->descriptor;
}

QT_END_NAMESPACE
