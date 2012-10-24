/****************************************************************************
**
** Copyright (C) 2012 Denis Shienkov <scapig@yandex.ru>
** Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
** Copyright (C) 2012 Andre Hartmann <aha_1980@gmx.de>
** Contact: http://www.qt-project.org/
**
** This file is part of the QtSerialPort module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "serialport_win_p.h"

#include <QtCore/qelapsedtimer.h>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtCore/qwineventnotifier.h>
#else
#include "qt4support/qwineventnotifier_p.h"
#endif

#include <QDebug>

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

QT_BEGIN_NAMESPACE_SERIALPORT

#ifndef Q_OS_WINCE

class CommEventNotifier : public QWinEventNotifier
{
public:
    CommEventNotifier(SerialPortPrivate *d, QObject *parent)
        : QWinEventNotifier(d->eventOverlapped.hEvent, parent)
        , dptr(d)
    {
    }

protected:
    virtual bool event(QEvent *e) {
        const bool ret = QWinEventNotifier::event(e);
        if (ret) {
            if (EV_ERR & dptr->eventMask)
                dptr->processIoErrors();
            if (EV_RXCHAR & dptr->eventMask) {
                if (!dptr->readSequenceStarted)
                    dptr->startAsyncRead();
            }
            ::WaitCommEvent(dptr->descriptor, &dptr->eventMask, &dptr->eventOverlapped);
        }
        return ret;
    }

private:
    SerialPortPrivate *dptr;
};

class ReadCompletionNotifier : public QWinEventNotifier
{
public:
    ReadCompletionNotifier(SerialPortPrivate *d, QObject *parent)
        : QWinEventNotifier(d->readOverlapped.hEvent, parent)
        , dptr(d)
    {}

protected:
    virtual bool event(QEvent *e) {
        bool ret = QWinEventNotifier::event(e);
        if (ret) {
            DWORD numberOfBytesTransferred = 0;
            BOOL success = ::GetOverlappedResult(dptr->descriptor,
                                                 &dptr->readOverlapped,
                                                 &numberOfBytesTransferred,
                                                 FALSE);
            if (success)
                dptr->completeAsyncRead(numberOfBytesTransferred);
        }
        return ret;
    }

private:
    SerialPortPrivate *dptr;
};

class WriteCompletionNotifier : public QWinEventNotifier
{
public:
    WriteCompletionNotifier(SerialPortPrivate *d, QObject *parent)
        : QWinEventNotifier(d->writeOverlapped.hEvent, parent)
        , dptr(d)
    {}

protected:
    virtual bool event(QEvent *e) {
        bool ret = QWinEventNotifier::event(e);
        if (ret) {
            DWORD numberOfBytesTransferred = 0;
            BOOL success = ::GetOverlappedResult(dptr->descriptor,
                                                 &dptr->writeOverlapped,
                                                 &numberOfBytesTransferred,
                                                 FALSE);
            if (success)
                dptr->completeAsyncWrite(numberOfBytesTransferred);
        }
        return ret;
    }

private:
    SerialPortPrivate *dptr;
};

SerialPortPrivate::SerialPortPrivate(SerialPort *q)
    : SerialPortPrivateData(q)
    , descriptor(INVALID_HANDLE_VALUE)
    , flagErrorFromCommEvent(false)
    , eventMask(EV_ERR)
    , eventNotifier(0)
    , readCompletionNotifier(0)
    , writeCompletionNotifier(0)
    , actualReadBufferSize(0)
    , actualWriteBufferSize(0)
    , readyReadEmitted(0)
    , readSequenceStarted(false)
    , writeSequenceStarted(false)
{
}

bool SerialPortPrivate::open(QIODevice::OpenMode mode)
{
    DWORD desiredAccess = 0;
    eventMask = EV_ERR;

    if (mode & QIODevice::ReadOnly) {
        desiredAccess |= GENERIC_READ;
        eventMask |= EV_RXCHAR;
    }
    if (mode & QIODevice::WriteOnly) {
        desiredAccess |= GENERIC_WRITE;
        eventMask |= EV_TXEMPTY;
    }

    descriptor = ::CreateFile(reinterpret_cast<const wchar_t*>(systemLocation.utf16()),
                              desiredAccess, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

    if (descriptor == INVALID_HANDLE_VALUE) {
        portError = decodeSystemError();
        return false;
    }

    if (!::GetCommState(descriptor, &restoredDcb)) {
        portError = decodeSystemError();
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
        portError = decodeSystemError();
        return false;
    }

    ::memset(&currentCommTimeouts, 0, sizeof(currentCommTimeouts));
    currentCommTimeouts.ReadIntervalTimeout = MAXDWORD;

    if (!updateCommTimeouts())
        return false;

    ::memset(&selectOverlapped, 0, sizeof(selectOverlapped));
    selectOverlapped.hEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);

    if (eventMask & EV_RXCHAR) {
        ::memset(&readOverlapped, 0, sizeof(readOverlapped));
        readOverlapped.hEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
        readCompletionNotifier = new ReadCompletionNotifier(this, q_ptr);
        readCompletionNotifier->setEnabled(true);
    }

    if (eventMask & EV_TXEMPTY) {
        // Disable EV_TXEMPTY for CommEventNotifier because not all serial ports
        // (such as from Bluetooth stack of Microsoft) supported this feature.
        // I.e. now do not use this event to write to the port.
        eventMask &= ~EV_TXEMPTY;
        ::memset(&writeOverlapped, 0, sizeof(writeOverlapped));
        writeOverlapped.hEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
        writeCompletionNotifier = new WriteCompletionNotifier(this, q_ptr);
        writeCompletionNotifier->setEnabled(true);
    }

    ::SetCommMask(descriptor, eventMask);
    ::memset(&eventOverlapped, 0, sizeof(eventOverlapped));
    eventOverlapped.hEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
    eventNotifier = new CommEventNotifier(this, q_ptr);
    eventNotifier->setEnabled(true);
    ::WaitCommEvent(descriptor, &eventMask, &eventOverlapped);

    detectDefaultSettings();
    return true;
}

void SerialPortPrivate::close()
{
    ::CancelIo(descriptor);

    if (eventNotifier) {
        eventNotifier->setEnabled(false);
        ::CancelIo(eventOverlapped.hEvent);
        ::CloseHandle(eventOverlapped.hEvent);
        eventNotifier->deleteLater();
        eventNotifier = 0;
    }

    if (readCompletionNotifier) {
        readCompletionNotifier->setEnabled(false);
        ::CancelIo(readOverlapped.hEvent);
        ::CloseHandle(readOverlapped.hEvent);
        readCompletionNotifier->deleteLater();
        readCompletionNotifier = 0;
    }

    if (readSequenceStarted)
        readSequenceStarted = false;

    readBuffer.clear();
    actualReadBufferSize = 0;

    if (writeCompletionNotifier) {
        writeCompletionNotifier->setEnabled(false);
        ::CancelIo(writeOverlapped.hEvent);
        ::CloseHandle(writeOverlapped.hEvent);
        writeCompletionNotifier->deleteLater();
        writeCompletionNotifier = 0;
    }

    if (writeSequenceStarted)
        writeSequenceStarted = false;

    writeBuffer.clear();
    actualWriteBufferSize = 0;

    readyReadEmitted = false;
    flagErrorFromCommEvent = false;

    if (restoreSettingsOnClose) {
        ::SetCommState(descriptor, &restoredDcb);
        ::SetCommTimeouts(descriptor, &restoredCommTimeouts);
    }

    ::CloseHandle(descriptor);
    descriptor = INVALID_HANDLE_VALUE;
}

#endif // #ifndef Q_OS_WINCE

SerialPort::Lines SerialPortPrivate::lines() const
{
    DWORD modemStat = 0;
    SerialPort::Lines ret = 0;

    if (!::GetCommModemStatus(descriptor, &modemStat))
        return ret;

    if (modemStat & MS_CTS_ON)
        ret |= SerialPort::Cts;
    if (modemStat & MS_DSR_ON)
        ret |= SerialPort::Dsr;
    if (modemStat & MS_RING_ON)
        ret |= SerialPort::Ri;
    if (modemStat & MS_RLSD_ON)
        ret |= SerialPort::Dcd;

    DWORD bytesReturned = 0;
    if (::DeviceIoControl(descriptor, IOCTL_SERIAL_GET_DTRRTS, NULL, 0,
                          &modemStat, sizeof(modemStat),
                          &bytesReturned, NULL)) {

        if (modemStat & SERIAL_DTR_STATE)
            ret |= SerialPort::Dtr;
        if (modemStat & SERIAL_RTS_STATE)
            ret |= SerialPort::Rts;
    }

    return ret;
}

bool SerialPortPrivate::setDtr(bool set)
{
    return ::EscapeCommFunction(descriptor, set ? SETDTR : CLRDTR);
}

bool SerialPortPrivate::setRts(bool set)
{
    return ::EscapeCommFunction(descriptor, set ? SETRTS : CLRRTS);
}

#ifndef Q_OS_WINCE

bool SerialPortPrivate::flush()
{
    return startAsyncWrite() && ::FlushFileBuffers(descriptor);
}

#endif

bool SerialPortPrivate::clear(SerialPort::Directions dir)
{
    DWORD flags = 0;
    if (dir & SerialPort::Input)
        flags |= PURGE_RXABORT | PURGE_RXCLEAR;
    if (dir & SerialPort::Output)
        flags |= PURGE_TXABORT | PURGE_TXCLEAR;
    return ::PurgeComm(descriptor, flags);
}

bool SerialPortPrivate::sendBreak(int duration)
{
    // FIXME:
    if (setBreak(true)) {
        ::Sleep(duration);
        if (setBreak(false))
            return true;
    }
    return false;
}

bool SerialPortPrivate::setBreak(bool set)
{
    if (set)
        return ::SetCommBreak(descriptor);
    return ::ClearCommBreak(descriptor);
}

qint64 SerialPortPrivate::bytesAvailable() const
{
    COMSTAT cs;
    ::memset(&cs, 0, sizeof(cs));
    if (!::ClearCommError(descriptor, NULL, &cs))
        return -1;
    return cs.cbInQue;
}

qint64 SerialPortPrivate::bytesToWrite() const
{
    COMSTAT cs;
    ::memset(&cs, 0, sizeof(cs));
    if (!::ClearCommError(descriptor, NULL, &cs))
        return -1;
    return cs.cbOutQue;
}

#ifndef Q_OS_WINCE

qint64 SerialPortPrivate::readFromBuffer(char *data, qint64 maxSize)
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

    if (!readSequenceStarted)
        startAsyncRead();

    return readSoFar;
}

qint64 SerialPortPrivate::writeToBuffer(const char *data, qint64 maxSize)
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

bool SerialPortPrivate::waitForReadyRead(int msecs)
{
    QElapsedTimer stopWatch;

    stopWatch.start();

    do {
        bool readyToStartRead = false;
        bool readyToStartWrite = false;
        bool readyToCompleteRead = false;
        bool readyToCompleteWrite = false;
        bool timedOut = false;
        if (!waitForReadOrWrite(&readyToStartRead, &readyToStartWrite,
                                &readyToCompleteRead, &readyToCompleteWrite,
                                true, !writeBuffer.isEmpty(),
                                timeoutValue(msecs, stopWatch.elapsed()), &timedOut)) {
            // This is occur timeout or another error
            // TODO: set error ?
            return false;
        }

        if (readyToStartRead) {
            if (!startAsyncRead())
                return false;
        }

        if (readyToStartWrite) {
            if (!startAsyncWrite(WriteChunkSize))
                return false;
        }

        DWORD bytesTransferred = 0;

        if (readyToCompleteRead) {
            if (!::GetOverlappedResult(descriptor, &readOverlapped,
                                       &bytesTransferred, FALSE)) {
                return false;
            }
            return completeAsyncRead(bytesTransferred);
        }

        if (readyToCompleteWrite) {
            if (::GetOverlappedResult(descriptor, &readOverlapped,
                                      &bytesTransferred, FALSE)) {
                completeAsyncWrite(bytesTransferred);
            }
        }

    } while (msecs == -1 || timeoutValue(msecs, stopWatch.elapsed()) > 0);
    return false;
}

bool SerialPortPrivate::waitForBytesWritten(int msecs)
{
    if (writeBuffer.isEmpty())
        return false;

    QElapsedTimer stopWatch;

    stopWatch.start();

    forever {
        bool readyToStartRead = false;
        bool readyToStartWrite = false;
        bool readyToCompleteRead = false;
        bool readyToCompleteWrite = false;
        bool timedOut = false;
        if (!waitForReadOrWrite(&readyToStartRead, &readyToStartWrite,
                                &readyToCompleteRead, &readyToCompleteWrite,
                                true, !writeBuffer.isEmpty(),
                                timeoutValue(msecs, stopWatch.elapsed()), &timedOut)) {
            // This is occur timeout or another error
            // TODO: set error ?
            return false;
        }

        if (readyToStartRead) {
            if (!startAsyncRead())
                return false;
        }

        if (readyToStartWrite) {
            startAsyncWrite(WriteChunkSize);
        }

        DWORD bytesTransferred = 0;

        if (readyToCompleteRead) {
            if (!::GetOverlappedResult(descriptor, &readOverlapped,
                                       &bytesTransferred, FALSE)) {
                return false;
            }
            if (!completeAsyncRead(bytesTransferred))
                return false;
        }

        if (readyToCompleteWrite) {
            if (::GetOverlappedResult(descriptor, &readOverlapped,
                                      &bytesTransferred, FALSE)) {
                if (completeAsyncWrite(bytesTransferred))
                    return true;
            }
        }

    }
    return false;
}

#endif // #ifndef Q_OS_WINCE

bool SerialPortPrivate::setRate(qint32 rate, SerialPort::Directions dir)
{
    if (dir != SerialPort::AllDirections) {
        portError = SerialPort::UnsupportedPortOperationError;
        return false;
    }
    currentDcb.BaudRate = rate;
    return updateDcb();
}

bool SerialPortPrivate::setDataBits(SerialPort::DataBits dataBits)
{
    currentDcb.ByteSize = dataBits;
    return updateDcb();
}

bool SerialPortPrivate::setParity(SerialPort::Parity parity)
{
    currentDcb.fParity = TRUE;
    switch (parity) {
    case SerialPort::NoParity:
        currentDcb.Parity = NOPARITY;
        currentDcb.fParity = FALSE;
        break;
    case SerialPort::OddParity:
        currentDcb.Parity = ODDPARITY;
        break;
    case SerialPort::EvenParity:
        currentDcb.Parity = EVENPARITY;
        break;
    case SerialPort::MarkParity:
        currentDcb.Parity = MARKPARITY;
        break;
    case SerialPort::SpaceParity:
        currentDcb.Parity = SPACEPARITY;
        break;
    default:
        currentDcb.Parity = NOPARITY;
        currentDcb.fParity = FALSE;
        break;
    }
    return updateDcb();
}

bool SerialPortPrivate::setStopBits(SerialPort::StopBits stopBits)
{
    switch (stopBits) {
    case SerialPort::OneStop:
        currentDcb.StopBits = ONESTOPBIT;
        break;
    case SerialPort::OneAndHalfStop:
        currentDcb.StopBits = ONE5STOPBITS;
        break;
    case SerialPort::TwoStop:
        currentDcb.StopBits = TWOSTOPBITS;
        break;
    default:
        currentDcb.StopBits = ONESTOPBIT;
        break;
    }
    return updateDcb();
}

bool SerialPortPrivate::setFlowControl(SerialPort::FlowControl flow)
{
    currentDcb.fInX = FALSE;
    currentDcb.fOutX = FALSE;
    currentDcb.fOutxCtsFlow = FALSE;
    currentDcb.fRtsControl = RTS_CONTROL_DISABLE;
    switch (flow) {
    case SerialPort::NoFlowControl:
        break;
    case SerialPort::SoftwareControl:
        currentDcb.fInX = TRUE;
        currentDcb.fOutX = TRUE;
        break;
    case SerialPort::HardwareControl:
        currentDcb.fOutxCtsFlow = TRUE;
        currentDcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
        break;
    default:
        break;
    }
    return updateDcb();
}

bool SerialPortPrivate::setDataErrorPolicy(SerialPort::DataErrorPolicy policy)
{
    policy = policy;
    return true;
}

#ifndef Q_OS_WINCE

bool SerialPortPrivate::startAsyncRead()
{
    DWORD bytesToRead = policy == SerialPort::IgnorePolicy ? ReadChunkSize : 1;

    if (readBufferMaxSize && bytesToRead > (readBufferMaxSize - readBuffer.size())) {
        bytesToRead = readBufferMaxSize - readBuffer.size();
        if (bytesToRead == 0) {
            // Buffer is full. User must read data from the buffer
            // before we can read more from the port.
            return false;
        }
    }

    char *ptr = readBuffer.reserve(bytesToRead);

    readSequenceStarted = true;
    if (::ReadFile(descriptor, ptr, bytesToRead, NULL, &readOverlapped))
        return true;

    switch (::GetLastError()) {
    case ERROR_IO_PENDING:
        // This is not an error. We're getting notified, when data arrives.
        return true;
    case ERROR_MORE_DATA:
        // This is not an error. The synchronous read succeeded.
        break;
    default:
        // error
        readSequenceStarted = false;
        return false;
    }

    return true;
}

bool SerialPortPrivate::startAsyncWrite(int maxSize)
{
    writeSequenceStarted = true;

    int nextSize = qMin(writeBuffer.nextDataBlockSize(), maxSize);

    const char *ptr = writeBuffer.readPointer();

    if (::WriteFile(descriptor, ptr, nextSize, NULL, &writeOverlapped))
        return true;

    switch (::GetLastError()) {
    case ERROR_IO_PENDING:
        // This is not an error. We're getting notified, when data arrives.
        return true;
    case ERROR_MORE_DATA:
        // This is not an error. The synchronous read succeeded.
        break;
    default:
        // error
        writeSequenceStarted = false;
        return false;
    }
    return true;
}

#endif // #ifndef Q_OS_WINCE

bool SerialPortPrivate::processIoErrors()
{
    DWORD error = 0;
    const bool ret = ::ClearCommError(descriptor, &error, FALSE);
    if (ret && error) {
        if (error & CE_FRAME)
            portError = SerialPort::FramingError;
        else if (error & CE_RXPARITY)
            portError = SerialPort::ParityError;
        else if (error & CE_BREAK)
            portError = SerialPort::BreakConditionError;
        else
            portError = SerialPort::UnknownPortError;

        flagErrorFromCommEvent = true;
    }
    return ret;
}

#ifndef Q_OS_WINCE

bool SerialPortPrivate::completeAsyncRead(DWORD numberOfBytes)
{
    actualReadBufferSize += qint64(numberOfBytes);
    readBuffer.truncate(actualReadBufferSize);

    if (numberOfBytes > 0) {

        // Process emulate policy.
        if (flagErrorFromCommEvent) {

            flagErrorFromCommEvent = false;

            // Ignore received character, remove it from buffer
            if (policy == SerialPort::SkipPolicy) {
                readSequenceStarted = false;
                readBuffer.getChar();
                startAsyncRead();
                return true;
            }

            // Abort receiving
            if (policy == SerialPort::StopReceivingPolicy) {
                readSequenceStarted = false;
                readyReadEmitted = true;
                emit q_ptr->readyRead();
                return true;
            }

            // Replace received character by zero
            if (policy == SerialPort::PassZeroPolicy) {
                readBuffer.getChar();
                readBuffer.putChar('\0');
            }

        }

        readyReadEmitted = true;
        emit q_ptr->readyRead();
        startAsyncRead();
    } else {
        readSequenceStarted = false;
    }
    return true;
}

bool SerialPortPrivate::completeAsyncWrite(DWORD numberOfBytes)
{
    writeBuffer.free(numberOfBytes);
    actualWriteBufferSize -= qint64(numberOfBytes);

    if (numberOfBytes > 0)
        emit q_ptr->bytesWritten(numberOfBytes);

    if (writeBuffer.isEmpty())
        writeSequenceStarted = false;
    else
        startAsyncWrite(WriteChunkSize);

    return true;
}

bool SerialPortPrivate::updateDcb()
{
    if (!::SetCommState(descriptor, &currentDcb)) {
        portError = decodeSystemError();
        return false;
    }
    return true;
}

bool SerialPortPrivate::updateCommTimeouts()
{
    if (!::SetCommTimeouts(descriptor, &currentCommTimeouts)) {
        portError = decodeSystemError();
        return false;
    }
    return true;
}

#endif // #ifndef Q_OS_WINCE

void SerialPortPrivate::detectDefaultSettings()
{
    // Detect rate.
    inputRate = quint32(currentDcb.BaudRate);
    outputRate = inputRate;

    // Detect databits.
    switch (currentDcb.ByteSize) {
    case 5:
        dataBits = SerialPort::Data5;
        break;
    case 6:
        dataBits = SerialPort::Data6;
        break;
    case 7:
        dataBits = SerialPort::Data7;
        break;
    case 8:
        dataBits = SerialPort::Data8;
        break;
    default:
        dataBits = SerialPort::UnknownDataBits;
        break;
    }

    // Detect parity.
    if ((currentDcb.Parity == NOPARITY) && !currentDcb.fParity)
        parity = SerialPort::NoParity;
    else if ((currentDcb.Parity == SPACEPARITY) && currentDcb.fParity)
        parity = SerialPort::SpaceParity;
    else if ((currentDcb.Parity == MARKPARITY) && currentDcb.fParity)
        parity = SerialPort::MarkParity;
    else if ((currentDcb.Parity == EVENPARITY) && currentDcb.fParity)
        parity = SerialPort::EvenParity;
    else if ((currentDcb.Parity == ODDPARITY) && currentDcb.fParity)
        parity = SerialPort::OddParity;
    else
        parity = SerialPort::UnknownParity;

    // Detect stopbits.
    switch (currentDcb.StopBits) {
    case ONESTOPBIT:
        stopBits = SerialPort::OneStop;
        break;
    case ONE5STOPBITS:
        stopBits = SerialPort::OneAndHalfStop;
        break;
    case TWOSTOPBITS:
        stopBits = SerialPort::TwoStop;
        break;
    default:
        stopBits = SerialPort::UnknownStopBits;
        break;
    }

    // Detect flow control.
    if (!currentDcb.fOutxCtsFlow && (currentDcb.fRtsControl == RTS_CONTROL_DISABLE)
            && !currentDcb.fInX && !currentDcb.fOutX) {
        flow = SerialPort::NoFlowControl;
    } else if (!currentDcb.fOutxCtsFlow && (currentDcb.fRtsControl == RTS_CONTROL_DISABLE)
               && currentDcb.fInX && currentDcb.fOutX) {
        flow = SerialPort::SoftwareControl;
    } else if (currentDcb.fOutxCtsFlow && (currentDcb.fRtsControl == RTS_CONTROL_HANDSHAKE)
               && !currentDcb.fInX && !currentDcb.fOutX) {
        flow = SerialPort::HardwareControl;
    } else
        flow = SerialPort::UnknownFlowControl;
}

SerialPort::PortError SerialPortPrivate::decodeSystemError() const
{
    SerialPort::PortError error;
    switch (::GetLastError()) {
    case ERROR_FILE_NOT_FOUND:
        error = SerialPort::NoSuchDeviceError;
        break;
    case ERROR_ACCESS_DENIED:
        error = SerialPort::PermissionDeniedError;
        break;
    case ERROR_INVALID_HANDLE:
        error = SerialPort::DeviceIsNotOpenedError;
        break;
    case ERROR_INVALID_PARAMETER:
        error = SerialPort::UnsupportedPortOperationError;
        break;
    default:
        error = SerialPort::UnknownPortError;
        break;
    }
    return error;
}

#ifndef Q_OS_WINCE

bool SerialPortPrivate::waitForReadOrWrite(bool *selectForStartRead, bool *selectForStartWrite,
                                           bool *selectForCompleteRead, bool *selectForCompleteWrite,
                                           bool checkRead, bool checkWrite,
                                           int msecs, bool *timedOut)
{
    Q_ASSERT(selectForStartRead);
    Q_ASSERT(selectForStartWrite);
    Q_ASSERT(selectForCompleteRead);
    Q_ASSERT(selectForCompleteWrite);

    DWORD eventMask = 0;

    if (!::WaitCommEvent(descriptor, &eventMask, &selectOverlapped)
            && ::GetLastError() != ERROR_IO_PENDING) {
        return false;
    }

    enum {
        SelectEventIndex = 0,
        ReadEventIndex = 1,
        WriteEventIndex = 2,
        NumberOfEvents = 3
    };

    HANDLE events[NumberOfEvents] = {
        selectOverlapped.hEvent,
        readOverlapped.hEvent,
        writeOverlapped.hEvent
    };

    DWORD waitResult = ::WaitForMultipleObjects(NumberOfEvents,
                                                events,
                                                FALSE, // wait any event
                                                qMax(msecs, 0));

    switch (waitResult) {
    case WAIT_OBJECT_0 + SelectEventIndex: {
        if (checkRead && (eventMask == EV_RXCHAR))
            *selectForStartRead = true;
        if (checkWrite && (eventMask == EV_TXEMPTY))
            *selectForStartWrite = true;
    }
        break;
    case WAIT_OBJECT_0 + ReadEventIndex: {
        if (checkRead)
            *selectForCompleteRead = true;
    }
        break;
    case WAIT_OBJECT_0 + WriteEventIndex: {
        if (checkWrite)
            *selectForCompleteWrite = true;
    }
        break;
    case WAIT_OBJECT_0 + WAIT_TIMEOUT: {
        *timedOut = true;
        return false;
    }
        break;
    default:
        return false;
    }

    return true;
}

static const QLatin1String defaultPathPrefix("\\\\.\\");

QString SerialPortPrivate::portNameToSystemLocation(const QString &port)
{
    QString ret = port;
    if (!ret.contains(defaultPathPrefix))
        ret.prepend(defaultPathPrefix);
    return ret;
}

QString SerialPortPrivate::portNameFromSystemLocation(const QString &location)
{
    QString ret = location;
    if (ret.contains(defaultPathPrefix))
        ret.remove(defaultPathPrefix);
    return ret;
}

#endif // #ifndef Q_OS_WINCE

// This table contains standard values of baud rates that
// are defined in MSDN and/or in Win SDK file winbase.h
static const qint32 standardRatesTable[] =
{
    #ifdef CBR_110
    CBR_110,
    #endif
    #ifdef CBR_300
    CBR_300,
    #endif
    #ifdef CBR_600
    CBR_600,
    #endif
    #ifdef CBR_1200
    CBR_1200,
    #endif
    #ifdef CBR_2400
    CBR_2400,
    #endif
    #ifdef CBR_4800
    CBR_4800,
    #endif
    #ifdef CBR_9600
    CBR_9600,
    #endif
    #ifdef CBR_14400
    CBR_14400,
    #endif
    #ifdef CBR_19200
    CBR_19200,
    #endif
    #ifdef CBR_38400
    CBR_38400,
    #endif
    #ifdef CBR_56000
    CBR_56000,
    #endif
    #ifdef CBR_57600
    CBR_57600,
    #endif
    #ifdef CBR_115200
    CBR_115200,
    #endif
    #ifdef CBR_128000
    CBR_128000,
    #endif
    #ifdef CBR_256000
    CBR_256000
    #endif
};

static const qint32 *standardRatesTable_end =
        standardRatesTable + sizeof(standardRatesTable)/sizeof(*standardRatesTable);

qint32 SerialPortPrivate::rateFromSetting(qint32 setting)
{
    const qint32 *ret = qFind(standardRatesTable, standardRatesTable_end, setting);
    return ret != standardRatesTable_end ? *ret : 0;
}

qint32 SerialPortPrivate::settingFromRate(qint32 rate)
{
    const qint32 *ret = qBinaryFind(standardRatesTable, standardRatesTable_end, rate);
    return ret != standardRatesTable_end ? *ret : 0;
}

QList<qint32> SerialPortPrivate::standardRates()
{
    QList<qint32> l;
    for (const qint32 *it = standardRatesTable; it != standardRatesTable_end; ++it)
        l.append(*it);
    return l;
}

QT_END_NAMESPACE_SERIALPORT
