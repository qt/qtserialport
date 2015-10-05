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
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qvector.h>
#include <QtCore/qtimer.h>
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

static void initializeOverlappedStructure(OVERLAPPED &overlapped)
{
    overlapped.Internal = 0;
    overlapped.InternalHigh = 0;
    overlapped.Offset = 0;
    overlapped.OffsetHigh = 0;
}

QSerialPortPrivate::QSerialPortPrivate(QSerialPort *q)
    : QSerialPortPrivateData(q)
    , handle(INVALID_HANDLE_VALUE)
    , readChunkBuffer(ReadChunkSize, 0)
    , writeStarted(false)
    , readStarted(false)
    , communicationNotifier(Q_NULLPTR)
    , readCompletionNotifier(Q_NULLPTR)
    , writeCompletionNotifier(Q_NULLPTR)
    , startAsyncWriteTimer(Q_NULLPTR)
    , originalEventMask(0)
    , triggeredEventMask(0)
    , actualBytesToWrite(0)
{
    communicationOverlapped.hEvent = Q_NULLPTR;
    readCompletionOverlapped.hEvent = Q_NULLPTR;
    writeCompletionOverlapped.hEvent = Q_NULLPTR;
}

QSerialPortPrivate::~QSerialPortPrivate()
{
    if (communicationOverlapped.hEvent)
        CloseHandle(communicationOverlapped.hEvent);
    if (readCompletionOverlapped.hEvent)
        CloseHandle(readCompletionOverlapped.hEvent);
    if (writeCompletionOverlapped.hEvent)
        CloseHandle(writeCompletionOverlapped.hEvent);
}

bool QSerialPortPrivate::open(QIODevice::OpenMode mode)
{
    DWORD desiredAccess = 0;
    originalEventMask = 0;

    if (mode & QIODevice::ReadOnly) {
        desiredAccess |= GENERIC_READ;
        originalEventMask |= EV_RXCHAR;
    }
    if (mode & QIODevice::WriteOnly)
        desiredAccess |= GENERIC_WRITE;

    handle = ::CreateFile(reinterpret_cast<const wchar_t*>(systemLocation.utf16()),
                              desiredAccess, 0, Q_NULLPTR, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, Q_NULLPTR);

    if (handle == INVALID_HANDLE_VALUE) {
        setError(getSystemError());
        return false;
    }

    if (initialize())
        return true;

    ::CloseHandle(handle);
    return false;
}

void QSerialPortPrivate::close()
{
    if (!::CancelIo(handle))
        setError(getSystemError());

    if (communicationNotifier) {
        delete communicationNotifier;
        communicationNotifier = Q_NULLPTR;
    }

    if (readCompletionNotifier) {
        delete readCompletionNotifier;
        readCompletionNotifier = Q_NULLPTR;
    }

    if (writeCompletionNotifier) {
        delete writeCompletionNotifier;
        writeCompletionNotifier = Q_NULLPTR;
    }

    if (startAsyncWriteTimer) {
        delete startAsyncWriteTimer;
        startAsyncWriteTimer = Q_NULLPTR;
    }

    readStarted = false;
    readBuffer.clear();

    writeStarted = false;
    writeBuffer.clear();
    actualBytesToWrite = 0;

    if (settingsRestoredOnClose) {
        if (!::SetCommState(handle, &restoredDcb))
            setError(getSystemError());
        else if (!::SetCommTimeouts(handle, &restoredCommTimeouts))
            setError(getSystemError());
    }

    if (!::CloseHandle(handle))
        setError(getSystemError());

    handle = INVALID_HANDLE_VALUE;
}

QSerialPort::PinoutSignals QSerialPortPrivate::pinoutSignals()
{
    DWORD modemStat = 0;

    if (!::GetCommModemStatus(handle, &modemStat)) {
        setError(getSystemError());
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
    if (!::DeviceIoControl(handle, IOCTL_SERIAL_GET_DTRRTS, Q_NULLPTR, 0,
                          &modemStat, sizeof(modemStat),
                          &bytesReturned, Q_NULLPTR)) {
        setError(getSystemError());
        return ret;
    }

    if (modemStat & SERIAL_DTR_STATE)
        ret |= QSerialPort::DataTerminalReadySignal;
    if (modemStat & SERIAL_RTS_STATE)
        ret |= QSerialPort::RequestToSendSignal;

    return ret;
}

bool QSerialPortPrivate::setDataTerminalReady(bool set)
{
    if (!::EscapeCommFunction(handle, set ? SETDTR : CLRDTR)) {
        setError(getSystemError());
        return false;
    }

    DCB dcb;
    if (!getDcb(&dcb))
        return false;

    dcb.fDtrControl = set ? DTR_CONTROL_ENABLE : DTR_CONTROL_DISABLE;
    return setDcb(&dcb);
}

bool QSerialPortPrivate::setRequestToSend(bool set)
{
    if (!::EscapeCommFunction(handle, set ? SETRTS : CLRRTS)) {
        setError(getSystemError());
        return false;
    }

    return true;
}

bool QSerialPortPrivate::flush()
{
    return _q_startAsyncWrite();
}

bool QSerialPortPrivate::clear(QSerialPort::Directions directions)
{
    DWORD flags = 0;
    if (directions & QSerialPort::Input)
        flags |= PURGE_RXABORT | PURGE_RXCLEAR;
    if (directions & QSerialPort::Output) {
        flags |= PURGE_TXABORT | PURGE_TXCLEAR;
        actualBytesToWrite = 0;
    }
    if (!::PurgeComm(handle, flags)) {
        setError(getSystemError());
        return false;
    }

    // We need start async read because a reading can be stalled. Since the
    // PurgeComm can abort of current reading sequence, or a port is in hardware
    // flow control mode, or a port has a limited read buffer size.
    if (directions & QSerialPort::Input)
        startAsyncCommunication();

    return true;
}

bool QSerialPortPrivate::sendBreak(int duration)
{
    if (!setBreakEnabled(true))
        return false;

    ::Sleep(duration);

    if (!setBreakEnabled(false))
        return false;

    return true;
}

bool QSerialPortPrivate::setBreakEnabled(bool set)
{
    if (set ? !::SetCommBreak(handle) : !::ClearCommBreak(handle)) {
        setError(getSystemError());
        return false;
    }

    return true;
}

qint64 QSerialPortPrivate::readData(char *data, qint64 maxSize)
{
    const qint64 result = readBuffer.read(data, maxSize);
    // We need try to start async reading to read a remainder from a driver's queue
    // in case we have a limited read buffer size. Because the read notification can
    // be stalled since Windows do not re-triggered an EV_RXCHAR event if a driver's
    // buffer has a remainder of data ready to read until a new data will be received.
    if (readBufferMaxSize
            && result > 0
            && (result == readBufferMaxSize || flowControl == QSerialPort::HardwareControl)) {
        startAsyncRead();
    }
    return result;
}

bool QSerialPortPrivate::waitForReadyRead(int msecs)
{
    if (!writeStarted && !_q_startAsyncWrite())
        return false;

    const qint64 initialReadBufferSize = readBuffer.size();
    qint64 currentReadBufferSize = initialReadBufferSize;

    QElapsedTimer stopWatch;
    stopWatch.start();

    do {
        HANDLE triggeredEvent = Q_NULLPTR;
        if (!waitAnyEvent(timeoutValue(msecs, stopWatch.elapsed()), &triggeredEvent) || !triggeredEvent)
            return false;

        if (triggeredEvent == communicationOverlapped.hEvent) {
            if (!_q_completeAsyncCommunication())
                return false;
        } else if (triggeredEvent == readCompletionOverlapped.hEvent) {
            if (!_q_completeAsyncRead())
                return false;
            const qint64 readBytesForOneReadOperation = qint64(readBuffer.size()) - currentReadBufferSize;
            if (readBytesForOneReadOperation == ReadChunkSize) {
                currentReadBufferSize = readBuffer.size();
            } else if (readBytesForOneReadOperation == 0) {
                if (initialReadBufferSize != currentReadBufferSize)
                    return true;
            } else {
                return true;
            }
        } else if (triggeredEvent == writeCompletionOverlapped.hEvent) {
            if (!_q_completeAsyncWrite())
                return false;
        } else {
            return false;
        }

    } while (msecs == -1 || timeoutValue(msecs, stopWatch.elapsed()) > 0);

    return false;
}

bool QSerialPortPrivate::waitForBytesWritten(int msecs)
{
    if (writeBuffer.isEmpty())
        return false;

    if (!writeStarted && !_q_startAsyncWrite())
        return false;

    QElapsedTimer stopWatch;
    stopWatch.start();

    forever {
        HANDLE triggeredEvent = Q_NULLPTR;
        if (!waitAnyEvent(timeoutValue(msecs, stopWatch.elapsed()), &triggeredEvent) || !triggeredEvent)
            return false;

        if (triggeredEvent == communicationOverlapped.hEvent) {
            if (!_q_completeAsyncCommunication())
                return false;
        } else if (triggeredEvent == readCompletionOverlapped.hEvent) {
            if (!_q_completeAsyncRead())
                return false;
        } else if (triggeredEvent == writeCompletionOverlapped.hEvent) {
            return _q_completeAsyncWrite();
        } else {
            return false;
        }

    }

    return false;
}

bool QSerialPortPrivate::setBaudRate()
{
    return setBaudRate(inputBaudRate, QSerialPort::AllDirections);
}

bool QSerialPortPrivate::setBaudRate(qint32 baudRate, QSerialPort::Directions directions)
{
    if (directions != QSerialPort::AllDirections) {
        setError(QSerialPortErrorInfo(QSerialPort::UnsupportedOperationError, QSerialPort::tr("Custom baud rate direction is unsupported")));
        return false;
    }

    DCB dcb;
    if (!getDcb(&dcb))
        return false;

    dcb.BaudRate = baudRate;
    return setDcb(&dcb);
}

bool QSerialPortPrivate::setDataBits(QSerialPort::DataBits dataBits)
{
    DCB dcb;
    if (!getDcb(&dcb))
        return false;

    dcb.ByteSize = dataBits;
    return setDcb(&dcb);
}

bool QSerialPortPrivate::setParity(QSerialPort::Parity parity)
{
    DCB dcb;
    if (!getDcb(&dcb))
        return false;

    dcb.fParity = TRUE;
    switch (parity) {
    case QSerialPort::NoParity:
        dcb.Parity = NOPARITY;
        dcb.fParity = FALSE;
        break;
    case QSerialPort::OddParity:
        dcb.Parity = ODDPARITY;
        break;
    case QSerialPort::EvenParity:
        dcb.Parity = EVENPARITY;
        break;
    case QSerialPort::MarkParity:
        dcb.Parity = MARKPARITY;
        break;
    case QSerialPort::SpaceParity:
        dcb.Parity = SPACEPARITY;
        break;
    default:
        dcb.Parity = NOPARITY;
        dcb.fParity = FALSE;
        break;
    }
    return setDcb(&dcb);
}

bool QSerialPortPrivate::setStopBits(QSerialPort::StopBits stopBits)
{
    DCB dcb;
    if (!getDcb(&dcb))
        return false;

    switch (stopBits) {
    case QSerialPort::OneStop:
        dcb.StopBits = ONESTOPBIT;
        break;
    case QSerialPort::OneAndHalfStop:
        dcb.StopBits = ONE5STOPBITS;
        break;
    case QSerialPort::TwoStop:
        dcb.StopBits = TWOSTOPBITS;
        break;
    default:
        dcb.StopBits = ONESTOPBIT;
        break;
    }
    return setDcb(&dcb);
}

bool QSerialPortPrivate::setFlowControl(QSerialPort::FlowControl flowControl)
{
    DCB dcb;
    if (!getDcb(&dcb))
        return false;

    dcb.fInX = FALSE;
    dcb.fOutX = FALSE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;
    switch (flowControl) {
    case QSerialPort::NoFlowControl:
        break;
    case QSerialPort::SoftwareControl:
        dcb.fInX = TRUE;
        dcb.fOutX = TRUE;
        break;
    case QSerialPort::HardwareControl:
        dcb.fOutxCtsFlow = TRUE;
        dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
        break;
    default:
        break;
    }
    return setDcb(&dcb);
}

bool QSerialPortPrivate::_q_completeAsyncCommunication()
{
    if (overlappedResult(communicationOverlapped) == qint64(-1))
        return false;

    return startAsyncRead();
}

bool QSerialPortPrivate::_q_completeAsyncRead()
{
    const qint64 bytesTransferred = overlappedResult(readCompletionOverlapped);
    if (bytesTransferred == qint64(-1)) {
        readStarted = false;
        return false;
    }
    if (bytesTransferred > 0)
        readBuffer.append(readChunkBuffer.left(bytesTransferred));

    readStarted = false;

    bool result = true;
    if (bytesTransferred == ReadChunkSize)
        result = startAsyncRead();
    else if (readBufferMaxSize == 0 || readBufferMaxSize > readBuffer.size())
        result = startAsyncCommunication();

    if (bytesTransferred > 0)
        emitReadyRead();

    return result;
}

bool QSerialPortPrivate::_q_completeAsyncWrite()
{
    Q_Q(QSerialPort);

    if (writeStarted) {
        const qint64 bytesTransferred = overlappedResult(writeCompletionOverlapped);
        if (bytesTransferred == qint64(-1)) {
            writeStarted = false;
            return false;
        } else if (bytesTransferred > 0) {
            writeBuffer.free(bytesTransferred);
            emit q->bytesWritten(bytesTransferred);
        }
        writeStarted = false;
    }

    return _q_startAsyncWrite();
}

bool QSerialPortPrivate::startAsyncCommunication()
{
    if (!setCommunicationNotificationEnabled(true))
        return false;

    initializeOverlappedStructure(communicationOverlapped);
    if (!::WaitCommEvent(handle, &triggeredEventMask, &communicationOverlapped)) {
        QSerialPortErrorInfo error = getSystemError();
        if (error.errorCode != QSerialPort::NoError) {
            if (error.errorCode == QSerialPort::PermissionError)
                error.errorCode = QSerialPort::ResourceError;
            setError(error);
            return false;
        }
    }
    return true;
}

bool QSerialPortPrivate::startAsyncRead()
{
    if (readStarted)
        return true;

    DWORD bytesToRead = ReadChunkSize;

    if (readBufferMaxSize && bytesToRead > (readBufferMaxSize - readBuffer.size())) {
        bytesToRead = readBufferMaxSize - readBuffer.size();
        if (bytesToRead == 0) {
            // Buffer is full. User must read data from the buffer
            // before we can read more from the port.
            return false;
        }
    }

    if (!setReadNotificationEnabled(true))
        return false;

    initializeOverlappedStructure(readCompletionOverlapped);
    if (::ReadFile(handle, readChunkBuffer.data(), bytesToRead, Q_NULLPTR, &readCompletionOverlapped)) {
        readStarted = true;
        return true;
    }

    QSerialPortErrorInfo error = getSystemError();
    if (error.errorCode != QSerialPort::NoError) {
        if (error.errorCode == QSerialPort::PermissionError)
            error.errorCode = QSerialPort::ResourceError;
        if (error.errorCode != QSerialPort::ResourceError)
            error.errorCode = QSerialPort::ReadError;
        setError(error);
        return false;
    }

    readStarted = true;
    return true;
}

bool QSerialPortPrivate::_q_startAsyncWrite()
{
    if (writeBuffer.isEmpty() || writeStarted)
        return true;

    if (!setWriteNotificationEnabled(true))
        return false;

    initializeOverlappedStructure(writeCompletionOverlapped);

    const int writeBytes = writeBuffer.nextDataBlockSize();
    if (!::WriteFile(handle, writeBuffer.readPointer(),
                     writeBytes,
                     Q_NULLPTR, &writeCompletionOverlapped)) {

        QSerialPortErrorInfo error = getSystemError();
        if (error.errorCode != QSerialPort::NoError) {
            if (error.errorCode != QSerialPort::ResourceError)
                error.errorCode = QSerialPort::WriteError;
            setError(error);
            return false;
        }
    }

    actualBytesToWrite -= writeBytes;
    writeStarted = true;
    return true;
}

void QSerialPortPrivate::emitReadyRead()
{
    Q_Q(QSerialPort);

    emit q->readyRead();
}

qint64 QSerialPortPrivate::writeData(const char *data, qint64 maxSize)
{
    Q_Q(QSerialPort);

    ::memcpy(writeBuffer.reserve(maxSize), data, maxSize);
    actualBytesToWrite += maxSize;

    if (!writeBuffer.isEmpty() && !writeStarted) {
        if (!startAsyncWriteTimer) {
            startAsyncWriteTimer = new QTimer(q);
            q->connect(startAsyncWriteTimer, SIGNAL(timeout()), q, SLOT(_q_startAsyncWrite()));
            startAsyncWriteTimer->setSingleShot(true);
        }
        startAsyncWriteTimer->start(0);
    }
    return maxSize;
}

inline bool QSerialPortPrivate::initialize()
{
    DCB dcb;
    if (!getDcb(&dcb))
        return false;

    restoredDcb = dcb;

    dcb.fBinary = TRUE;
    dcb.fInX = FALSE;
    dcb.fOutX = FALSE;
    dcb.fAbortOnError = FALSE;
    dcb.fNull = FALSE;
    dcb.fErrorChar = FALSE;

    if (dcb.fDtrControl == DTR_CONTROL_HANDSHAKE)
        dcb.fDtrControl = DTR_CONTROL_DISABLE;

    dcb.BaudRate = inputBaudRate;

    if (!setDcb(&dcb))
        return false;

    if (!::GetCommTimeouts(handle, &restoredCommTimeouts)) {
        setError(getSystemError());
        return false;
    }

    ::ZeroMemory(&currentCommTimeouts, sizeof(currentCommTimeouts));
    currentCommTimeouts.ReadIntervalTimeout = MAXDWORD;

    if (!updateCommTimeouts())
        return false;

    if (!::SetCommMask(handle, originalEventMask)) {
        setError(getSystemError());
        return false;
    }

    if ((originalEventMask & EV_RXCHAR) && !startAsyncCommunication())
        return false;

    return true;
}

bool QSerialPortPrivate::setDcb(DCB *dcb)
{
    if (!::SetCommState(handle, dcb)) {
        setError(getSystemError());
        return false;
    }
    return true;
}

bool QSerialPortPrivate::getDcb(DCB *dcb)
{
    ::ZeroMemory(dcb, sizeof(DCB));
    dcb->DCBlength = sizeof(DCB);

    if (!::GetCommState(handle, dcb)) {
        setError(getSystemError());
        return false;
    }
    return true;
}

bool QSerialPortPrivate::updateCommTimeouts()
{
    if (!::SetCommTimeouts(handle, &currentCommTimeouts)) {
        setError(getSystemError());
        return false;
    }
    return true;
}

qint64 QSerialPortPrivate::overlappedResult(OVERLAPPED &overlapped)
{
    DWORD bytesTransferred = 0;
    if (!::GetOverlappedResult(handle, &overlapped, &bytesTransferred, FALSE)) {
        QSerialPortErrorInfo error = getSystemError();
        if (error.errorCode == QSerialPort::NoError)
            return qint64(0);
        if (error.errorCode != QSerialPort::ResourceError) {
            if (&overlapped == &readCompletionOverlapped)
                error.errorCode = QSerialPort::ReadError;
            else if (&overlapped == &writeCompletionOverlapped)
                error.errorCode = QSerialPort::WriteError;
            setError(error);
            return qint64(-1);
        }
    }
    return bytesTransferred;
}

QSerialPortErrorInfo QSerialPortPrivate::getSystemError(int systemErrorCode) const
{
    if (systemErrorCode == -1)
        systemErrorCode = ::GetLastError();

    QSerialPortErrorInfo error;
    error.errorString = qt_error_string(systemErrorCode);

    switch (systemErrorCode) {
    case ERROR_SUCCESS:
        error.errorCode = QSerialPort::NoError;
        break;
    case ERROR_IO_PENDING:
        error.errorCode = QSerialPort::NoError;
        break;
    case ERROR_MORE_DATA:
        error.errorCode = QSerialPort::NoError;
        break;
    case ERROR_FILE_NOT_FOUND:
        error.errorCode = QSerialPort::DeviceNotFoundError;
        break;
    case ERROR_PATH_NOT_FOUND:
        error.errorCode = QSerialPort::DeviceNotFoundError;
        break;
    case ERROR_INVALID_NAME:
        error.errorCode = QSerialPort::DeviceNotFoundError;
        break;
    case ERROR_ACCESS_DENIED:
        error.errorCode = QSerialPort::PermissionError;
        break;
    case ERROR_INVALID_HANDLE:
        error.errorCode = QSerialPort::ResourceError;
        break;
    case ERROR_INVALID_PARAMETER:
        error.errorCode = QSerialPort::UnsupportedOperationError;
        break;
    case ERROR_BAD_COMMAND:
        error.errorCode = QSerialPort::ResourceError;
        break;
    case ERROR_DEVICE_REMOVED:
        error.errorCode = QSerialPort::ResourceError;
        break;
    case ERROR_OPERATION_ABORTED:
        error.errorCode = QSerialPort::ResourceError;
        break;
    case WAIT_TIMEOUT:
        error.errorCode = QSerialPort::TimeoutError;
        break;
    default:
        error.errorCode = QSerialPort::UnknownError;
        break;
    }
    return error;
}

bool QSerialPortPrivate::waitAnyEvent(int msecs, HANDLE *triggeredEvent)
{
    QVector<HANDLE> handles;

    if (communicationOverlapped.hEvent)
        handles.append(communicationOverlapped.hEvent);
    if (readCompletionOverlapped.hEvent)
        handles.append(readCompletionOverlapped.hEvent);
    if (writeCompletionOverlapped.hEvent)
        handles.append(writeCompletionOverlapped.hEvent);

    DWORD waitResult = ::WaitForMultipleObjects(handles.count(),
                                                handles.constData(),
                                                FALSE, // wait any event
                                                msecs == -1 ? INFINITE : msecs);
    if (waitResult == WAIT_TIMEOUT) {
        setError(getSystemError(WAIT_TIMEOUT));
        return false;
    }
    if (waitResult >= DWORD(WAIT_OBJECT_0 + handles.count())) {
        setError(getSystemError());
        return false;
    }

    *triggeredEvent = handles.at(waitResult - WAIT_OBJECT_0);
    return true;
}

bool QSerialPortPrivate::setCommunicationNotificationEnabled(bool enable)
{
    Q_Q(QSerialPort);

    if (communicationNotifier) {
        communicationNotifier->setEnabled(enable);
    } else if (enable) {
        communicationOverlapped.hEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
        if (!communicationOverlapped.hEvent) {
            setError(getSystemError());
            return false;
        }
        communicationNotifier = new QWinEventNotifier(q);
        q->connect(communicationNotifier, SIGNAL(activated(HANDLE)),
                   q, SLOT(_q_completeAsyncCommunication()));
        communicationNotifier->setHandle(communicationOverlapped.hEvent);
        communicationNotifier->setEnabled(true);
    }

    return true;
}

bool QSerialPortPrivate::setReadNotificationEnabled(bool enable)
{
    Q_Q(QSerialPort);

    if (readCompletionNotifier) {
        readCompletionNotifier->setEnabled(enable);
    } else if (enable) {
        readCompletionOverlapped.hEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
        if (!readCompletionOverlapped.hEvent) {
            setError(getSystemError());
            return false;
        }
        readCompletionNotifier = new QWinEventNotifier(q);
        q->connect(readCompletionNotifier, SIGNAL(activated(HANDLE)),
                   q, SLOT(_q_completeAsyncRead()));
        readCompletionNotifier->setHandle(readCompletionOverlapped.hEvent);
        readCompletionNotifier->setEnabled(true);
    }

    return true;
}

bool QSerialPortPrivate::setWriteNotificationEnabled(bool enable)
{
    Q_Q(QSerialPort);

    if (writeCompletionNotifier) {
        writeCompletionNotifier->setEnabled(enable);
    } else if (enable) {
        writeCompletionOverlapped.hEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
        if (!writeCompletionOverlapped.hEvent) {
            setError(getSystemError());
            return false;
        }
        writeCompletionNotifier = new QWinEventNotifier(q);
        q->connect(writeCompletionNotifier, SIGNAL(activated(HANDLE)),
                   q, SLOT(_q_completeAsyncWrite()));
        writeCompletionNotifier->setHandle(writeCompletionOverlapped.hEvent);
        writeCompletionNotifier->setEnabled(true);
    }

    return true;
}

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

qint32 QSerialPortPrivate::settingFromBaudRate(qint32 baudRate)
{
    const QList<qint32> baudRatePairList = standardBaudRatePairList();
    const QList<qint32>::const_iterator baudRatePairListConstIterator
            = std::find(baudRatePairList.constBegin(), baudRatePairList.constEnd(), baudRate);

    return (baudRatePairListConstIterator != baudRatePairList.constEnd()) ? *baudRatePairListConstIterator : 0;
}

QList<qint32> QSerialPortPrivate::standardBaudRates()
{
    return standardBaudRatePairList();
}

QSerialPort::Handle QSerialPort::handle() const
{
    Q_D(const QSerialPort);
    return d->handle;
}

QT_END_NAMESPACE
