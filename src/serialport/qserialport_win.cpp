/****************************************************************************
**
** Copyright (C) 2012 Denis Shienkov <denis.shienkov@gmail.com>
** Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
** Copyright (C) 2012 Andre Hartmann <aha_1980@gmx.de>
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtSerialPort module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qserialport_p.h"

#include <QtCore/qcoreevent.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qvector.h>
#include <QtCore/qtimer.h>
#include <private/qwinoverlappedionotifier_p.h>
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

bool QSerialPortPrivate::open(QIODevice::OpenMode mode)
{
    Q_Q(QSerialPort);

    DWORD desiredAccess = 0;
    originalEventMask = EV_ERR;

    if (mode & QIODevice::ReadOnly) {
        desiredAccess |= GENERIC_READ;
        originalEventMask |= EV_RXCHAR;
    }
    if (mode & QIODevice::WriteOnly)
        desiredAccess |= GENERIC_WRITE;

    handle = ::CreateFile(reinterpret_cast<const wchar_t*>(systemLocation.utf16()),
                              desiredAccess, 0, Q_NULLPTR, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, Q_NULLPTR);

    if (handle == INVALID_HANDLE_VALUE) {
        q->setError(decodeSystemError());
        return false;
    }

    if (initialize())
        return true;

    ::CloseHandle(handle);
    return false;
}

void QSerialPortPrivate::close()
{
    Q_Q(QSerialPort);

    if (!::CancelIo(handle))
        q->setError(decodeSystemError());

    if (notifier)
        notifier->deleteLater();

    readStarted = false;
    writeStarted = false;
    writeBuffer.clear();
    actualBytesToWrite = 0;

    parityErrorOccurred = false;

    if (settingsRestoredOnClose) {
        if (!::SetCommState(handle, &restoredDcb))
            q->setError(decodeSystemError());
        else if (!::SetCommTimeouts(handle, &restoredCommTimeouts))
            q->setError(decodeSystemError());
    }

    if (!::CloseHandle(handle))
        q->setError(decodeSystemError());

    handle = INVALID_HANDLE_VALUE;
}

QSerialPort::PinoutSignals QSerialPortPrivate::pinoutSignals()
{
    Q_Q(QSerialPort);

    DWORD modemStat = 0;

    if (!::GetCommModemStatus(handle, &modemStat)) {
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
    if (!::DeviceIoControl(handle, IOCTL_SERIAL_GET_DTRRTS, Q_NULLPTR, 0,
                          &modemStat, sizeof(modemStat),
                          &bytesReturned, Q_NULLPTR)) {
        q->setError(decodeSystemError());
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
    Q_Q(QSerialPort);

    if (!::EscapeCommFunction(handle, set ? SETDTR : CLRDTR)) {
        q->setError(decodeSystemError());
        return false;
    }

    currentDcb.fDtrControl = set ? DTR_CONTROL_ENABLE : DTR_CONTROL_DISABLE;
    return true;
}

bool QSerialPortPrivate::setRequestToSend(bool set)
{
    Q_Q(QSerialPort);

    if (!::EscapeCommFunction(handle, set ? SETRTS : CLRRTS)) {
        q->setError(decodeSystemError());
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
    Q_Q(QSerialPort);

    DWORD flags = 0;
    if (directions & QSerialPort::Input)
        flags |= PURGE_RXABORT | PURGE_RXCLEAR;
    if (directions & QSerialPort::Output) {
        flags |= PURGE_TXABORT | PURGE_TXCLEAR;
        actualBytesToWrite = 0;
    }
    if (!::PurgeComm(handle, flags)) {
        q->setError(decodeSystemError());
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
    Q_Q(QSerialPort);

    if (set ? !::SetCommBreak(handle) : !::ClearCommBreak(handle)) {
        q->setError(decodeSystemError());
        return false;
    }

    return true;
}

qint64 QSerialPortPrivate::readData(char *data, qint64 maxSize)
{
    Q_UNUSED(data);
    Q_UNUSED(maxSize);

    // We need try to start async reading to read a remainder from a driver's queue
    // in case we have a limited read buffer size. Because the read notification can
    // be stalled since Windows do not re-triggered an EV_RXCHAR event if a driver's
    // buffer has a remainder of data ready to read until a new data will be received.
    if (readBufferMaxSize || flowControl == QSerialPort::HardwareControl)
        startAsyncRead();

    // return 0 indicating there may be more data in the future
    return qint64(0);
}

bool QSerialPortPrivate::waitForReadyRead(int msecs)
{
    if (!writeStarted && !_q_startAsyncWrite())
        return false;

    const qint64 initialReadBufferSize = buffer.size();
    qint64 currentReadBufferSize = initialReadBufferSize;

    QElapsedTimer stopWatch;
    stopWatch.start();

    do {
        OVERLAPPED *overlapped = waitForNotified(timeoutValue(msecs, stopWatch.elapsed()));
        if (!overlapped)
            return false;

        const qint64 bytesTransferred = overlappedResult(overlapped);

        if (overlapped == &communicationOverlapped) {
            if (!completeAsyncCommunication(bytesTransferred))
                return false;
        } else if (overlapped == &readCompletionOverlapped) {
            if (!completeAsyncRead(bytesTransferred))
                return false;
            const qint64 readBytesForOneReadOperation = qint64(buffer.size()) - currentReadBufferSize;
            if (readBytesForOneReadOperation == ReadChunkSize) {
                currentReadBufferSize = buffer.size();
            } else if (readBytesForOneReadOperation == 0) {
                if (initialReadBufferSize != currentReadBufferSize)
                    return true;
            } else {
                return true;
            }
        } else if (overlapped == &writeCompletionOverlapped) {
            if (!completeAsyncWrite(bytesTransferred))
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
        OVERLAPPED *overlapped = waitForNotified(timeoutValue(msecs, stopWatch.elapsed()));
        if (!overlapped)
            return false;

        const qint64 bytesTransferred = overlappedResult(overlapped);

        if (overlapped == &communicationOverlapped) {
            if (!completeAsyncCommunication(bytesTransferred))
                return false;
        } else if (overlapped == &readCompletionOverlapped) {
            if (!completeAsyncRead(bytesTransferred))
                return false;
        } else if (overlapped == &writeCompletionOverlapped) {
            return completeAsyncWrite(bytesTransferred);
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

bool QSerialPortPrivate::setFlowControl(QSerialPort::FlowControl flowControl)
{
    currentDcb.fInX = FALSE;
    currentDcb.fOutX = FALSE;
    currentDcb.fOutxCtsFlow = FALSE;
    currentDcb.fRtsControl = RTS_CONTROL_DISABLE;
    switch (flowControl) {
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

bool QSerialPortPrivate::completeAsyncCommunication(qint64 bytesTransferred)
{
    if (bytesTransferred == qint64(-1))
        return false;
    if (EV_ERR & triggeredEventMask)
        handleLineStatusErrors();

    return startAsyncRead();
}

bool QSerialPortPrivate::completeAsyncRead(qint64 bytesTransferred)
{
    if (bytesTransferred == qint64(-1)) {
        readStarted = false;
        return false;
    }
    if (bytesTransferred > 0) {
        char *ptr = buffer.reserve(bytesTransferred);
        ::memcpy(ptr, readChunkBuffer.constData(), bytesTransferred);
        if (!emulateErrorPolicy())
            emitReadyRead();
    }

    readStarted = false;

    if ((bytesTransferred == ReadChunkSize) && (policy == QSerialPort::IgnorePolicy))
        return startAsyncRead();
    else if (readBufferMaxSize == 0 || readBufferMaxSize > buffer.size())
        return startAsyncCommunication();
    else
        return true;
}

bool QSerialPortPrivate::completeAsyncWrite(qint64 bytesTransferred)
{
    Q_Q(QSerialPort);

    if (writeStarted) {
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
    Q_Q(QSerialPort);

    ::ZeroMemory(&communicationOverlapped, sizeof(communicationOverlapped));
    if (!::WaitCommEvent(handle, &triggeredEventMask, &communicationOverlapped)) {
        QSerialPort::SerialPortError error = decodeSystemError();
        if (error != QSerialPort::NoError) {
            if (error == QSerialPort::PermissionError)
                error = QSerialPort::ResourceError;
            q->setError(error);
            return false;
        }
    }
    return true;
}

bool QSerialPortPrivate::startAsyncRead()
{
    Q_Q(QSerialPort);

    if (readStarted)
        return true;

    DWORD bytesToRead = policy == QSerialPort::IgnorePolicy ? ReadChunkSize : 1;

    if (readBufferMaxSize && bytesToRead > (readBufferMaxSize - buffer.size())) {
        bytesToRead = readBufferMaxSize - buffer.size();
        if (bytesToRead == 0) {
            // Buffer is full. User must read data from the buffer
            // before we can read more from the port.
            return false;
        }
    }

    ::ZeroMemory(&readCompletionOverlapped, sizeof(readCompletionOverlapped));
    if (::ReadFile(handle, readChunkBuffer.data(), bytesToRead, Q_NULLPTR, &readCompletionOverlapped)) {
        readStarted = true;
        return true;
    }

    QSerialPort::SerialPortError error = decodeSystemError();
    if (error != QSerialPort::NoError) {
        if (error == QSerialPort::PermissionError)
            error = QSerialPort::ResourceError;
        if (error != QSerialPort::ResourceError)
            error = QSerialPort::ReadError;
        q->setError(error);
        return false;
    }

    readStarted = true;
    return true;
}

bool QSerialPortPrivate::_q_startAsyncWrite()
{
    Q_Q(QSerialPort);

    if (writeBuffer.isEmpty() || writeStarted)
        return true;

    const int writeBytes = writeBuffer.nextDataBlockSize();
    ::ZeroMemory(&writeCompletionOverlapped, sizeof(writeCompletionOverlapped));
    if (!::WriteFile(handle, writeBuffer.readPointer(),
                     writeBytes, Q_NULLPTR, &writeCompletionOverlapped)) {

        QSerialPort::SerialPortError error = decodeSystemError();
        if (error != QSerialPort::NoError) {
            if (error != QSerialPort::ResourceError)
                error = QSerialPort::WriteError;
            q->setError(error);
            return false;
        }
    }

    actualBytesToWrite -= writeBytes;
    writeStarted = true;
    return true;
}

void QSerialPortPrivate::_q_notified(DWORD numberOfBytes, DWORD errorCode, OVERLAPPED *overlapped)
{
    Q_Q(QSerialPort);

    const QSerialPort::SerialPortError error = decodeSystemError(errorCode);
    if (error != QSerialPort::NoError) {
        q->setError(error);
        return;
    }

    if (overlapped == &communicationOverlapped)
        completeAsyncCommunication(numberOfBytes);
    else if (overlapped == &readCompletionOverlapped)
        completeAsyncRead(numberOfBytes);
    else if (overlapped == &writeCompletionOverlapped)
        completeAsyncWrite(numberOfBytes);
    else
        Q_ASSERT(!"Unknown OVERLAPPED activated");
}

bool QSerialPortPrivate::emulateErrorPolicy()
{
    if (!parityErrorOccurred)
        return false;

    parityErrorOccurred = false;

    switch (policy) {
    case QSerialPort::SkipPolicy:
        buffer.getChar();
        break;
    case QSerialPort::PassZeroPolicy:
        buffer.getChar();
        buffer.ungetChar('\0');
        emitReadyRead();
        break;
    case QSerialPort::IgnorePolicy:
        return false;
    case QSerialPort::StopReceivingPolicy:
        emitReadyRead();
        break;
    default:
        return false;
    }

    return true;
}

void QSerialPortPrivate::emitReadyRead()
{
    Q_Q(QSerialPort);

    emit q->readyRead();
}

qint64 QSerialPortPrivate::bytesToWrite() const
{
    return actualBytesToWrite;
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

void QSerialPortPrivate::handleLineStatusErrors()
{
    Q_Q(QSerialPort);

    DWORD errors = 0;
    if (!::ClearCommError(handle, &errors, Q_NULLPTR)) {
        q->setError(decodeSystemError());
        return;
    }

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

OVERLAPPED *QSerialPortPrivate::waitForNotified(int msecs)
{
    Q_Q(QSerialPort);

    OVERLAPPED *overlapped = notifier->waitForAnyNotified(msecs);
    if (!overlapped) {
        q->setError(decodeSystemError(WAIT_TIMEOUT));
        return 0;
    }
    return overlapped;
}

inline bool QSerialPortPrivate::initialize()
{
    Q_Q(QSerialPort);

    ::ZeroMemory(&restoredDcb, sizeof(restoredDcb));
    restoredDcb.DCBlength = sizeof(restoredDcb);

    if (!::GetCommState(handle, &restoredDcb)) {
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

    if (currentDcb.fDtrControl ==  DTR_CONTROL_HANDSHAKE)
        currentDcb.fDtrControl = DTR_CONTROL_DISABLE;

    if (!updateDcb())
        return false;

    if (!::GetCommTimeouts(handle, &restoredCommTimeouts)) {
        q->setError(decodeSystemError());
        return false;
    }

    ::ZeroMemory(&currentCommTimeouts, sizeof(currentCommTimeouts));
    currentCommTimeouts.ReadIntervalTimeout = MAXDWORD;

    if (!updateCommTimeouts())
        return false;

    if (!::SetCommMask(handle, originalEventMask)) {
        q->setError(decodeSystemError());
        return false;
    }

    notifier = new QWinOverlappedIoNotifier(q);
    q->connect(notifier, SIGNAL(notified(quint32, quint32, OVERLAPPED*)),
               q, SLOT(_q_notified(quint32, quint32, OVERLAPPED*)),
               Qt::QueuedConnection);
    notifier->setHandle(handle);
    notifier->setEnabled(true);

    if (!startAsyncCommunication())
        return false;

    return true;
}

bool QSerialPortPrivate::updateDcb()
{
    Q_Q(QSerialPort);

    if (!::SetCommState(handle, &currentDcb)) {
        q->setError(decodeSystemError());
        return false;
    }
    return true;
}

bool QSerialPortPrivate::updateCommTimeouts()
{
    Q_Q(QSerialPort);

    if (!::SetCommTimeouts(handle, &currentCommTimeouts)) {
        q->setError(decodeSystemError());
        return false;
    }
    return true;
}

qint64 QSerialPortPrivate::overlappedResult(OVERLAPPED *overlapped)
{
    Q_Q(QSerialPort);

    DWORD bytesTransferred = 0;
    if (!::GetOverlappedResult(handle, overlapped, &bytesTransferred, FALSE)) {
        const QSerialPort::SerialPortError error = decodeSystemError();
        if (error == QSerialPort::NoError)
            return qint64(0);
        if (error != QSerialPort::ResourceError) {
            if (overlapped == &readCompletionOverlapped)
                q->setError(QSerialPort::ReadError);
            else if (overlapped == &writeCompletionOverlapped)
                q->setError(QSerialPort::WriteError);
            else
                q->setError(error);
            return qint64(-1);
        }
    }
    return bytesTransferred;
}

QSerialPort::SerialPortError QSerialPortPrivate::decodeSystemError(int systemErrorCode) const
{
    if (systemErrorCode == -1)
        systemErrorCode = ::GetLastError();

    QSerialPort::SerialPortError error;
    switch (systemErrorCode) {
    case ERROR_SUCCESS:
        error = QSerialPort::NoError;
        break;
    case ERROR_IO_PENDING:
        error = QSerialPort::NoError;
        break;
    case ERROR_MORE_DATA:
        error = QSerialPort::NoError;
        break;
    case ERROR_FILE_NOT_FOUND:
        error = QSerialPort::DeviceNotFoundError;
        break;
    case ERROR_INVALID_NAME:
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
    case ERROR_OPERATION_ABORTED:
        error = QSerialPort::ResourceError;
        break;
    case WAIT_TIMEOUT:
        error = QSerialPort::TimeoutError;
        break;
    default:
        error = QSerialPort::UnknownError;
        break;
    }
    return error;
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
    return standardBaudRatePairList();
}

QSerialPort::Handle QSerialPort::handle() const
{
    Q_D(const QSerialPort);
    return d->handle;
}

QT_END_NAMESPACE
