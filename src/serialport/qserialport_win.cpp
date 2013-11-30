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

#include "qserialport_win_p.h"

#include <QtCore/qcoreevent.h>

#ifndef Q_OS_WINCE
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qvector.h>
#endif

#include <QtCore/qwineventnotifier.h>
#include <algorithm>

#ifndef CTL_CODE
#  define CTL_CODE(DeviceType, Function, Method, Access) ( \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
    )
#endif

#ifndef FILE_DEVICE_SERIAL_PORT
#  define FILE_DEVICE_SERIAL_PORT  27
#endif

#ifndef METHOD_BUFFERED
#  define METHOD_BUFFERED  0
#endif

#ifndef FILE_ANY_ACCESS
#  define FILE_ANY_ACCESS  0x00000000
#endif

#ifndef IOCTL_SERIAL_GET_DTRRTS
#  define IOCTL_SERIAL_GET_DTRRTS \
    CTL_CODE(FILE_DEVICE_SERIAL_PORT, 30, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

#ifndef SERIAL_DTR_STATE
#  define SERIAL_DTR_STATE  0x00000001
#endif

#ifndef SERIAL_RTS_STATE
#  define SERIAL_RTS_STATE  0x00000002
#endif

QT_BEGIN_NAMESPACE

#ifndef Q_OS_WINCE

class AbstractOverlappedEventNotifier : public QWinEventNotifier
{
    Q_OBJECT
public:
    enum Type { CommEvent, ReadCompletionEvent, WriteCompletionEvent };

    AbstractOverlappedEventNotifier(QSerialPortPrivate *d, Type type, bool manual, QObject *parent)
        : QWinEventNotifier(parent), dptr(d), t(type) {
        ::memset(&o, 0, sizeof(o));
        o.hEvent = ::CreateEvent(NULL, manual, FALSE, NULL);
        setHandle(o.hEvent);
        dptr->notifiers[o.hEvent] = this;
    }

    virtual bool processCompletionRoutine() = 0;

    virtual ~AbstractOverlappedEventNotifier() {
        setEnabled(false);
        ::CloseHandle(o.hEvent);
    }

    Type type() const { return t; }
    OVERLAPPED *overlappedPointer() { return &o; }

protected:
    bool event(QEvent *e) Q_DECL_OVERRIDE {
        const bool ret = QWinEventNotifier::event(e);
        if (e->type() == QEvent::WinEventAct)
            processCompletionRoutine();
        return ret;
    }

    QSerialPortPrivate *dptr;
    Type t;
    OVERLAPPED o;
};

class CommOverlappedEventNotifier : public AbstractOverlappedEventNotifier
{
    Q_OBJECT
public:
    CommOverlappedEventNotifier(QSerialPortPrivate *d, DWORD eventMask, QObject *parent)
        : AbstractOverlappedEventNotifier(d, CommEvent, false, parent)
        , originalEventMask(eventMask), triggeredEventMask(0) {
        ::SetCommMask(dptr->descriptor, originalEventMask);
        startWaitCommEvent();
    }

    void startWaitCommEvent() { ::WaitCommEvent(dptr->descriptor, &triggeredEventMask, &o); }

    bool processCompletionRoutine() Q_DECL_OVERRIDE {
        DWORD numberOfBytesTransferred = 0;
        ::GetOverlappedResult(dptr->descriptor, &o, &numberOfBytesTransferred, FALSE);

        bool error = false;

        // Check for unexpected event. This event triggered when pulled previously
        // opened device from the system, when opened as for not to read and not to
        // write options and so forth.
        if (triggeredEventMask == 0)
            error = true;

        // Workaround for standard CDC ACM serial ports, for which triggered an
        // unexpected event EV_TXEMPTY at data transmission.
        if ((originalEventMask & triggeredEventMask) == 0) {
            if ((triggeredEventMask & EV_TXEMPTY) == 0)
                error = true;
        }

        // Start processing a caught error.
        if (error || (EV_ERR & triggeredEventMask))
            dptr->processIoErrors(error);

        if (!error)
            dptr->startAsyncRead();

        return !error;
    }

private:
    DWORD originalEventMask;
    DWORD triggeredEventMask;
};

class ReadOverlappedCompletionNotifier : public AbstractOverlappedEventNotifier
{
    Q_OBJECT
public:
    ReadOverlappedCompletionNotifier(QSerialPortPrivate *d, QObject *parent)
        : AbstractOverlappedEventNotifier(d, ReadCompletionEvent, false, parent) {}

    bool processCompletionRoutine() Q_DECL_OVERRIDE {
        DWORD numberOfBytesTransferred = 0;
        ::GetOverlappedResult(dptr->descriptor, &o, &numberOfBytesTransferred, FALSE);
        dptr->completeAsyncRead(numberOfBytesTransferred);

        // start async read for possible remainder into driver queue
        if ((numberOfBytesTransferred > 0) && (dptr->policy == QSerialPort::IgnorePolicy)) {
            dptr->startAsyncRead();
        } else { // driver queue is emplty, so startup wait comm event
            CommOverlappedEventNotifier *n =
                    qobject_cast<CommOverlappedEventNotifier *>(dptr->lookupCommEventNotifier());
            if (n)
                n->startWaitCommEvent();
        }

        return true;
    }
};

class WriteOverlappedCompletionNotifier : public AbstractOverlappedEventNotifier
{
    Q_OBJECT
public:
    WriteOverlappedCompletionNotifier(QSerialPortPrivate *d, QObject *parent)
        : AbstractOverlappedEventNotifier(d, WriteCompletionEvent, false, parent) {}

    bool processCompletionRoutine() Q_DECL_OVERRIDE {
        setEnabled(false);
        DWORD numberOfBytesTransferred = 0;
        ::GetOverlappedResult(dptr->descriptor, &o, &numberOfBytesTransferred, FALSE);
        dptr->completeAsyncWrite(numberOfBytesTransferred);
        return true;
    }
};

#include "qserialport_win.moc"

QSerialPortPrivate::QSerialPortPrivate(QSerialPort *q)
    : QSerialPortPrivateData(q)
    , descriptor(INVALID_HANDLE_VALUE)
    , parityErrorOccurred(false)
    , actualReadBufferSize(0)
    , actualWriteBufferSize(0)
    , acyncWritePosition(0)
    , readyReadEmitted(0)
    , writeSequenceStarted(false)
{
}

bool QSerialPortPrivate::open(QIODevice::OpenMode mode)
{
    Q_Q(QSerialPort);

    DWORD desiredAccess = 0;
    DWORD originalEventMask = EV_ERR;

    if (mode & QIODevice::ReadOnly) {
        desiredAccess |= GENERIC_READ;
        originalEventMask |= EV_RXCHAR;
    }
    if (mode & QIODevice::WriteOnly)
        desiredAccess |= GENERIC_WRITE;

    descriptor = ::CreateFile(reinterpret_cast<const wchar_t*>(systemLocation.utf16()),
                              desiredAccess, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

    if (descriptor == INVALID_HANDLE_VALUE) {
        q->setError(decodeSystemError());
        return false;
    }

    if (!::GetCommState(descriptor, &restoredDcb)) {
        q->setError(decodeSystemError());
        return false;
    }

    currentDcb = restoredDcb;
    currentDcb.fBinary = TRUE;
    currentDcb.fInX = FALSE;
    currentDcb.fOutX = FALSE;
    currentDcb.fAbortOnError = FALSE;
    currentDcb.fNull = FALSE;
    currentDcb.fErrorChar = FALSE;

    if (!updateDcb())
        return false;

    if (!::GetCommTimeouts(descriptor, &restoredCommTimeouts)) {
        q->setError(decodeSystemError());
        return false;
    }

    ::memset(&currentCommTimeouts, 0, sizeof(currentCommTimeouts));
    currentCommTimeouts.ReadIntervalTimeout = MAXDWORD;

    if (!updateCommTimeouts())
        return false;

    if (originalEventMask & EV_RXCHAR) {
        QWinEventNotifier *n = new ReadOverlappedCompletionNotifier(this, q);
        n->setEnabled(true);
    }

    QWinEventNotifier *n = new CommOverlappedEventNotifier(this, originalEventMask, q);
    n->setEnabled(true);

    detectDefaultSettings();
    return true;
}

void QSerialPortPrivate::close()
{
    ::CancelIo(descriptor);

    qDeleteAll(notifiers);
    notifiers.clear();

    readBuffer.clear();
    actualReadBufferSize = 0;

    writeSequenceStarted = false;
    writeBuffer.clear();
    actualWriteBufferSize = 0;
    acyncWritePosition = 0;

    readyReadEmitted = false;
    parityErrorOccurred = false;

    if (settingsRestoredOnClose) {
        ::SetCommState(descriptor, &restoredDcb);
        ::SetCommTimeouts(descriptor, &restoredCommTimeouts);
    }

    ::CloseHandle(descriptor);
    descriptor = INVALID_HANDLE_VALUE;
}

#endif // #ifndef Q_OS_WINCE

QSerialPort::PinoutSignals QSerialPortPrivate::pinoutSignals()
{
    Q_Q(QSerialPort);

    DWORD modemStat = 0;

    if (!::GetCommModemStatus(descriptor, &modemStat)) {
        q->setError(decodeSystemError());
        return QSerialPort::NoSignal;
    }

    QSerialPort::PinoutSignals ret = QSerialPort::NoSignal;

    if (modemStat & MS_CTS_ON)
        ret |= QSerialPort::ClearToSendSignal;
    if (modemStat & MS_DSR_ON)
        ret |= QSerialPort::DataSetReadySignal;
    if (modemStat & MS_RING_ON)
        ret |= QSerialPort::RingIndicatorSignal;
    if (modemStat & MS_RLSD_ON)
        ret |= QSerialPort::DataCarrierDetectSignal;

    DWORD bytesReturned = 0;
    if (::DeviceIoControl(descriptor, IOCTL_SERIAL_GET_DTRRTS, NULL, 0,
                          &modemStat, sizeof(modemStat),
                          &bytesReturned, NULL)) {

        if (modemStat & SERIAL_DTR_STATE)
            ret |= QSerialPort::DataTerminalReadySignal;
        if (modemStat & SERIAL_RTS_STATE)
            ret |= QSerialPort::RequestToSendSignal;
    }

    return ret;
}

bool QSerialPortPrivate::setDataTerminalReady(bool set)
{
    return ::EscapeCommFunction(descriptor, set ? SETDTR : CLRDTR);
}

bool QSerialPortPrivate::setRequestToSend(bool set)
{
    return ::EscapeCommFunction(descriptor, set ? SETRTS : CLRRTS);
}

#ifndef Q_OS_WINCE

bool QSerialPortPrivate::flush()
{
    return startAsyncWrite() && ::FlushFileBuffers(descriptor);
}

bool QSerialPortPrivate::clear(QSerialPort::Directions directions)
{
    DWORD flags = 0;
    if (directions & QSerialPort::Input) {
        flags |= PURGE_RXABORT | PURGE_RXCLEAR;
        actualReadBufferSize = 0;
    }
    if (directions & QSerialPort::Output) {
        flags |= PURGE_TXABORT | PURGE_TXCLEAR;
        actualWriteBufferSize = 0;
        acyncWritePosition = 0;
        writeSequenceStarted = false;
    }
    return ::PurgeComm(descriptor, flags);
}

#endif

bool QSerialPortPrivate::sendBreak(int duration)
{
    // FIXME:
    if (setBreakEnabled(true)) {
        ::Sleep(duration);
        if (setBreakEnabled(false))
            return true;
    }
    return false;
}

bool QSerialPortPrivate::setBreakEnabled(bool set)
{
    if (set)
        return ::SetCommBreak(descriptor);
    return ::ClearCommBreak(descriptor);
}

qint64 QSerialPortPrivate::systemInputQueueSize () const
{
    COMSTAT cs;
    ::memset(&cs, 0, sizeof(cs));
    if (!::ClearCommError(descriptor, NULL, &cs))
        return -1;
    return cs.cbInQue;
}

qint64 QSerialPortPrivate::systemOutputQueueSize () const
{
    COMSTAT cs;
    ::memset(&cs, 0, sizeof(cs));
    if (!::ClearCommError(descriptor, NULL, &cs))
        return -1;
    return cs.cbOutQue;
}

#ifndef Q_OS_WINCE

qint64 QSerialPortPrivate::bytesAvailable() const
{
    return actualReadBufferSize;
}

qint64 QSerialPortPrivate::readFromBuffer(char *data, qint64 maxSize)
{
    if (actualReadBufferSize == 0)
        return 0;

    qint64 readSoFar = -1;
    if (maxSize == 1 && actualReadBufferSize > 0) {
        *data = readBuffer.getChar();
        actualReadBufferSize--;
        readSoFar = 1;
    } else {
        const qint64 bytesToRead = qMin(qint64(actualReadBufferSize), maxSize);
        readSoFar = 0;
        while (readSoFar < bytesToRead) {
            const char *ptr = readBuffer.readPointer();
            const int bytesToReadFromThisBlock = qMin(bytesToRead - readSoFar,
                                                      qint64(readBuffer.nextDataBlockSize()));
            ::memcpy(data + readSoFar, ptr, bytesToReadFromThisBlock);
            readSoFar += bytesToReadFromThisBlock;
            readBuffer.free(bytesToReadFromThisBlock);
            actualReadBufferSize -= bytesToReadFromThisBlock;
        }
    }

    return readSoFar;
}

qint64 QSerialPortPrivate::writeToBuffer(const char *data, qint64 maxSize)
{
    char *ptr = writeBuffer.reserve(maxSize);
    if (maxSize == 1) {
        *ptr = *data;
        actualWriteBufferSize++;
    } else {
        ::memcpy(ptr, data, maxSize);
        actualWriteBufferSize += maxSize;
    }

    if (!writeSequenceStarted)
        startAsyncWrite(WriteChunkSize);

    return maxSize;
}

bool QSerialPortPrivate::waitForReadyRead(int msecs)
{
    Q_Q(QSerialPort);

    QElapsedTimer stopWatch;
    stopWatch.start();

    do {
        bool timedOut = false;
        AbstractOverlappedEventNotifier *n = 0;

        if (!waitAnyEvent(timeoutValue(msecs, stopWatch.elapsed()), &timedOut, &n) || !n) {
            // This is occur timeout or another error
            if (!timedOut)
                q->setError(decodeSystemError());
            return false;
        }

        switch (n->type()) {
        case AbstractOverlappedEventNotifier::CommEvent:
            if (!n->processCompletionRoutine())
                return false;
            break;
        case AbstractOverlappedEventNotifier::ReadCompletionEvent:
            return n->processCompletionRoutine();
        case AbstractOverlappedEventNotifier::WriteCompletionEvent:
            n->processCompletionRoutine();
            break;
        default: // newer called
            return false;
        }
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
        bool timedOut = false;
        AbstractOverlappedEventNotifier *n = 0;

        if (!waitAnyEvent(timeoutValue(msecs, stopWatch.elapsed()), &timedOut, &n) || !n) {
            if (!timedOut)
                q->setError(decodeSystemError());
            return false;
        }

        switch (n->type()) {
        case AbstractOverlappedEventNotifier::CommEvent:
            // do nothing, jump to ReadCompletionEvent case
        case AbstractOverlappedEventNotifier::ReadCompletionEvent:
            n->processCompletionRoutine();
            break;
        case AbstractOverlappedEventNotifier::WriteCompletionEvent:
            return n->processCompletionRoutine();
        default: // newer called
            return false;
        }
    }

    return false;
}

#endif // #ifndef Q_OS_WINCE

bool QSerialPortPrivate::setBaudRate(qint32 baudRate, QSerialPort::Directions directions)
{
    Q_Q(QSerialPort);

    if (directions != QSerialPort::AllDirections) {
        q->setError(QSerialPort::UnsupportedOperationError);
        return false;
    }
    currentDcb.BaudRate = baudRate;
    return updateDcb();
}

bool QSerialPortPrivate::setDataBits(QSerialPort::DataBits dataBits)
{
    currentDcb.ByteSize = dataBits;
    return updateDcb();
}

bool QSerialPortPrivate::setParity(QSerialPort::Parity parity)
{
    currentDcb.fParity = TRUE;
    switch (parity) {
    case QSerialPort::NoParity:
        currentDcb.Parity = NOPARITY;
        currentDcb.fParity = FALSE;
        break;
    case QSerialPort::OddParity:
        currentDcb.Parity = ODDPARITY;
        break;
    case QSerialPort::EvenParity:
        currentDcb.Parity = EVENPARITY;
        break;
    case QSerialPort::MarkParity:
        currentDcb.Parity = MARKPARITY;
        break;
    case QSerialPort::SpaceParity:
        currentDcb.Parity = SPACEPARITY;
        break;
    default:
        currentDcb.Parity = NOPARITY;
        currentDcb.fParity = FALSE;
        break;
    }
    return updateDcb();
}

bool QSerialPortPrivate::setStopBits(QSerialPort::StopBits stopBits)
{
    switch (stopBits) {
    case QSerialPort::OneStop:
        currentDcb.StopBits = ONESTOPBIT;
        break;
    case QSerialPort::OneAndHalfStop:
        currentDcb.StopBits = ONE5STOPBITS;
        break;
    case QSerialPort::TwoStop:
        currentDcb.StopBits = TWOSTOPBITS;
        break;
    default:
        currentDcb.StopBits = ONESTOPBIT;
        break;
    }
    return updateDcb();
}

bool QSerialPortPrivate::setFlowControl(QSerialPort::FlowControl flow)
{
    currentDcb.fInX = FALSE;
    currentDcb.fOutX = FALSE;
    currentDcb.fOutxCtsFlow = FALSE;
    currentDcb.fRtsControl = RTS_CONTROL_DISABLE;
    switch (flow) {
    case QSerialPort::NoFlowControl:
        break;
    case QSerialPort::SoftwareControl:
        currentDcb.fInX = TRUE;
        currentDcb.fOutX = TRUE;
        break;
    case QSerialPort::HardwareControl:
        currentDcb.fOutxCtsFlow = TRUE;
        currentDcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
        break;
    default:
        break;
    }
    return updateDcb();
}

bool QSerialPortPrivate::setDataErrorPolicy(QSerialPort::DataErrorPolicy policy)
{
    policy = policy;
    return true;
}

#ifndef Q_OS_WINCE

bool QSerialPortPrivate::startAsyncRead()
{
    Q_Q(QSerialPort);

    DWORD bytesToRead = policy == QSerialPort::IgnorePolicy ? ReadChunkSize : 1;

    if (readBufferMaxSize && bytesToRead > (readBufferMaxSize - readBuffer.size())) {
        bytesToRead = readBufferMaxSize - readBuffer.size();
        if (bytesToRead == 0) {
            // Buffer is full. User must read data from the buffer
            // before we can read more from the port.
            return false;
        }
    }

    AbstractOverlappedEventNotifier *n = lookupReadCompletionNotifier();
    if (!n) {
        q->setError(QSerialPort::ResourceError);
        return false;
    }

    char *ptr = readBuffer.reserve(bytesToRead);

    if (::ReadFile(descriptor, ptr, bytesToRead, NULL, n->overlappedPointer()))
        return true;

    QSerialPort::SerialPortError error = decodeSystemError();
    if (error != QSerialPort::NoError) {
        if (error != QSerialPort::ResourceError)
            error = QSerialPort::ReadError;
        q->setError(error);

        readBuffer.truncate(actualReadBufferSize);
        return false;
    }

    return true;
}

bool QSerialPortPrivate::startAsyncWrite(int maxSize)
{
    Q_Q(QSerialPort);

    qint64 nextSize = 0;
    const char *ptr = writeBuffer.readPointerAtPosition(acyncWritePosition, nextSize);

    nextSize = qMin(nextSize, qint64(maxSize));
    acyncWritePosition += nextSize;

    // no more data to write
    if (!ptr || nextSize == 0)
        return true;

    writeSequenceStarted = true;

    AbstractOverlappedEventNotifier *n = lookupFreeWriteCompletionNotifier();
    if (!n) {
        q->setError(QSerialPort::ResourceError);
        return false;
    }

    n->setEnabled(true);

    if (::WriteFile(descriptor, ptr, nextSize, NULL, n->overlappedPointer()))
        return true;

    QSerialPort::SerialPortError error = decodeSystemError();
    if (error != QSerialPort::NoError) {
         writeSequenceStarted = false;

         if (error != QSerialPort::ResourceError)
             error = QSerialPort::WriteError;

         q->setError(error);
         return false;
    }

    return true;
}

#endif // #ifndef Q_OS_WINCE

void QSerialPortPrivate::processIoErrors(bool error)
{
    Q_Q(QSerialPort);

    if (error) {
        q->setError(QSerialPort::ResourceError);
        return;
    }

    DWORD errors = 0;
    if (::ClearCommError(descriptor, &errors, NULL) && errors) {
        if (errors & CE_FRAME) {
            q->setError(QSerialPort::FramingError);
        } else if (errors & CE_RXPARITY) {
            q->setError(QSerialPort::ParityError);
            parityErrorOccurred = true;
        } else if (errors & CE_BREAK) {
            q->setError(QSerialPort::BreakConditionError);
        } else {
            q->setError(QSerialPort::UnknownError);
        }
    }
}

#ifndef Q_OS_WINCE

void QSerialPortPrivate::completeAsyncRead(DWORD numberOfBytes)
{
    Q_Q(QSerialPort);

    actualReadBufferSize += qint64(numberOfBytes);
    readBuffer.truncate(actualReadBufferSize);

    if (numberOfBytes > 0) {

        // Process emulate policy.
        if ((policy != QSerialPort::IgnorePolicy) && parityErrorOccurred) {

            parityErrorOccurred = false;

            // Ignore received character, remove it from buffer
            if (policy == QSerialPort::SkipPolicy) {
                readBuffer.getChar();
                // Force returning without emitting a readyRead() signal
                return;
            }

            // Abort receiving
            if (policy == QSerialPort::StopReceivingPolicy) {
                readyReadEmitted = true;
                emit q->readyRead();
                return;
            }

            // Replace received character by zero
            if (policy == QSerialPort::PassZeroPolicy) {
                readBuffer.getChar();
                readBuffer.putChar('\0');
            }

        }

        readyReadEmitted = true;
        emit q->readyRead();
    }
}

void QSerialPortPrivate::completeAsyncWrite(DWORD numberOfBytes)
{
    Q_Q(QSerialPort);

    writeBuffer.free(numberOfBytes);
    actualWriteBufferSize -= qint64(numberOfBytes);
    acyncWritePosition -= qint64(numberOfBytes);

    if (numberOfBytes > 0)
        emit q->bytesWritten(numberOfBytes);

    if (writeBuffer.isEmpty())
        writeSequenceStarted = false;
    else
        startAsyncWrite(WriteChunkSize);
}

AbstractOverlappedEventNotifier *QSerialPortPrivate::lookupFreeWriteCompletionNotifier()
{
    Q_Q(QSerialPort);

    // find first free not running write notifier
    foreach (AbstractOverlappedEventNotifier *n, notifiers) {
        if ((n->type() == AbstractOverlappedEventNotifier::WriteCompletionEvent)
                && !n->isEnabled()) {
            return n;
        }
    }
    // if all write notifiers in use, then create new write notifier
    return new WriteOverlappedCompletionNotifier(this, q);
}

AbstractOverlappedEventNotifier *QSerialPortPrivate::lookupCommEventNotifier()
{
    foreach (AbstractOverlappedEventNotifier *n, notifiers) {
        if (n->type() == AbstractOverlappedEventNotifier::CommEvent)
            return n;
    }
    return 0;
}

AbstractOverlappedEventNotifier *QSerialPortPrivate::lookupReadCompletionNotifier()
{
    foreach (AbstractOverlappedEventNotifier *n, notifiers) {
        if (n->type() == AbstractOverlappedEventNotifier::ReadCompletionEvent)
            return n;
    }
    return 0;
}

bool QSerialPortPrivate::updateDcb()
{
    Q_Q(QSerialPort);

    if (!::SetCommState(descriptor, &currentDcb)) {
        q->setError(decodeSystemError());
        return false;
    }
    return true;
}

bool QSerialPortPrivate::updateCommTimeouts()
{
    Q_Q(QSerialPort);

    if (!::SetCommTimeouts(descriptor, &currentCommTimeouts)) {
        q->setError(decodeSystemError());
        return false;
    }
    return true;
}

#endif // #ifndef Q_OS_WINCE

void QSerialPortPrivate::detectDefaultSettings()
{
    // Detect baud rate.
    inputBaudRate = quint32(currentDcb.BaudRate);
    outputBaudRate = inputBaudRate;

    // Detect databits.
    switch (currentDcb.ByteSize) {
    case 5:
        dataBits = QSerialPort::Data5;
        break;
    case 6:
        dataBits = QSerialPort::Data6;
        break;
    case 7:
        dataBits = QSerialPort::Data7;
        break;
    case 8:
        dataBits = QSerialPort::Data8;
        break;
    default:
        qWarning("%s: Unexpected data bits settings", Q_FUNC_INFO);
        dataBits = QSerialPort::Data8;
        break;
    }

    // Detect parity.
    if ((currentDcb.Parity == NOPARITY) && !currentDcb.fParity)
        parity = QSerialPort::NoParity;
    else if ((currentDcb.Parity == SPACEPARITY) && currentDcb.fParity)
        parity = QSerialPort::SpaceParity;
    else if ((currentDcb.Parity == MARKPARITY) && currentDcb.fParity)
        parity = QSerialPort::MarkParity;
    else if ((currentDcb.Parity == EVENPARITY) && currentDcb.fParity)
        parity = QSerialPort::EvenParity;
    else if ((currentDcb.Parity == ODDPARITY) && currentDcb.fParity)
        parity = QSerialPort::OddParity;
    else {
        qWarning("%s: Unexpected parity settings", Q_FUNC_INFO);
        parity = QSerialPort::NoParity;
    }

    // Detect stopbits.
    switch (currentDcb.StopBits) {
    case ONESTOPBIT:
        stopBits = QSerialPort::OneStop;
        break;
    case ONE5STOPBITS:
        stopBits = QSerialPort::OneAndHalfStop;
        break;
    case TWOSTOPBITS:
        stopBits = QSerialPort::TwoStop;
        break;
    default:
        qWarning("%s: Unexpected stop bits settings", Q_FUNC_INFO);
        stopBits = QSerialPort::OneStop;
        break;
    }

    // Detect flow control.
    if (!currentDcb.fOutxCtsFlow && (currentDcb.fRtsControl == RTS_CONTROL_DISABLE)
            && !currentDcb.fInX && !currentDcb.fOutX) {
        flow = QSerialPort::NoFlowControl;
    } else if (!currentDcb.fOutxCtsFlow && (currentDcb.fRtsControl == RTS_CONTROL_DISABLE)
               && currentDcb.fInX && currentDcb.fOutX) {
        flow = QSerialPort::SoftwareControl;
    } else if (currentDcb.fOutxCtsFlow && (currentDcb.fRtsControl == RTS_CONTROL_HANDSHAKE)
               && !currentDcb.fInX && !currentDcb.fOutX) {
        flow = QSerialPort::HardwareControl;
    } else {
        qWarning("%s: Unexpected flow control settings", Q_FUNC_INFO);
        flow = QSerialPort::NoFlowControl;
    }
}

QSerialPort::SerialPortError QSerialPortPrivate::decodeSystemError() const
{
    QSerialPort::SerialPortError error;
    switch (::GetLastError()) {
    case ERROR_IO_PENDING:
        error = QSerialPort::NoError;
        break;
    case ERROR_MORE_DATA:
        error = QSerialPort::NoError;
        break;
    case ERROR_FILE_NOT_FOUND:
        error = QSerialPort::DeviceNotFoundError;
        break;
    case ERROR_ACCESS_DENIED:
        error = QSerialPort::PermissionError;
        break;
    case ERROR_INVALID_HANDLE:
        error = QSerialPort::ResourceError;
        break;
    case ERROR_INVALID_PARAMETER:
        error = QSerialPort::UnsupportedOperationError;
        break;
    case ERROR_BAD_COMMAND:
        error = QSerialPort::ResourceError;
        break;
    case ERROR_DEVICE_REMOVED:
        error = QSerialPort::ResourceError;
        break;
    default:
        error = QSerialPort::UnknownError;
        break;
    }
    return error;
}

#ifndef Q_OS_WINCE

bool QSerialPortPrivate::waitAnyEvent(int msecs, bool *timedOut,
                                     AbstractOverlappedEventNotifier **triggeredNotifier)
{
    Q_Q(QSerialPort);

    Q_ASSERT(timedOut);

    QVector<HANDLE> handles =  notifiers.keys().toVector();
    DWORD waitResult = ::WaitForMultipleObjects(handles.count(),
                                                handles.constData(),
                                                FALSE, // wait any event
                                                msecs == -1 ? INFINITE : msecs);
    if (waitResult == WAIT_TIMEOUT) {
        *timedOut = true;
        q->setError(QSerialPort::TimeoutError);
        return false;
    }

    if (waitResult >= DWORD(WAIT_OBJECT_0 + handles.count()))
        return false;

    HANDLE h = handles.at(waitResult - WAIT_OBJECT_0);
    *triggeredNotifier = notifiers.value(h);
    return true;
}

static const QString defaultPathPrefix = QStringLiteral("\\\\.\\");

QString QSerialPortPrivate::portNameToSystemLocation(const QString &port)
{
    QString ret = port;
    if (!ret.contains(defaultPathPrefix))
        ret.prepend(defaultPathPrefix);
    return ret;
}

QString QSerialPortPrivate::portNameFromSystemLocation(const QString &location)
{
    QString ret = location;
    if (ret.contains(defaultPathPrefix))
        ret.remove(defaultPathPrefix);
    return ret;
}

#endif // #ifndef Q_OS_WINCE

// This table contains standard values of baud rates that
// are defined in MSDN and/or in Win SDK file winbase.h

static const QList<qint32> standardBaudRatePairList()
{

    static const QList<qint32> standardBaudRatesTable = QList<qint32>()

        #ifdef CBR_110
            << CBR_110
        #endif

        #ifdef CBR_300
            << CBR_300
        #endif

        #ifdef CBR_600
            << CBR_600
        #endif

        #ifdef CBR_1200
            << CBR_1200
        #endif

        #ifdef CBR_2400
            << CBR_2400
        #endif

        #ifdef CBR_4800
            << CBR_4800
        #endif

        #ifdef CBR_9600
            << CBR_9600
        #endif

        #ifdef CBR_14400
            << CBR_14400
        #endif

        #ifdef CBR_19200
            << CBR_19200
        #endif

        #ifdef CBR_38400
            << CBR_38400
        #endif

        #ifdef CBR_56000
            << CBR_56000
        #endif

        #ifdef CBR_57600
            << CBR_57600
        #endif

        #ifdef CBR_115200
            << CBR_115200
        #endif

        #ifdef CBR_128000
            << CBR_128000
        #endif

        #ifdef CBR_256000
            << CBR_256000
        #endif
    ;

    return standardBaudRatesTable;
};

qint32 QSerialPortPrivate::baudRateFromSetting(qint32 setting)
{
    const QList<qint32> baudRatePairs = standardBaudRatePairList();
    const QList<qint32>::const_iterator baudRatePairListConstIterator
            = std::find(baudRatePairs.constBegin(), baudRatePairs.constEnd(), setting);

    return (baudRatePairListConstIterator != baudRatePairs.constEnd()) ? *baudRatePairListConstIterator : 0;
}

qint32 QSerialPortPrivate::settingFromBaudRate(qint32 baudRate)
{
    const QList<qint32> baudRatePairList = standardBaudRatePairList();
    const QList<qint32>::const_iterator baudRatePairListConstIterator
            = std::find(baudRatePairList.constBegin(), baudRatePairList.constEnd(), baudRate);

    return (baudRatePairListConstIterator != baudRatePairList.constEnd()) ? *baudRatePairListConstIterator : 0;
}

QList<qint32> QSerialPortPrivate::standardBaudRates()
{
    QList<qint32> ret;
    const QList<qint32> baudRatePairs = standardBaudRatePairList();

    foreach (qint32 baudRatePair, baudRatePairs) {
        ret.append(baudRatePair);
    }

    return ret;
}

QSerialPort::Handle QSerialPort::handle() const
{
    Q_D(const QSerialPort);
    return d->descriptor;
}

QT_END_NAMESPACE
