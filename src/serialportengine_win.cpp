/*
    License...
*/

/*!
    \class WinSerialPortEngine
    \internal

    \brief The WinSerialPortEngine class provides windows OS
    platform-specific low level access to a serial port.

    \reentrant
    \ingroup serialport-main
    \inmodule QtSerialPort

    Currently the class supports as NT-based OS (Win 2K/XP/Vista/7),
    and as various embedded WinCE.

    WinSerialPortEngine (as well as other platform-dependent engines)
    is a class with multiple inheritance, which on the one hand,
    derives from a general abstract class interface SerialPortEngine,
    on the other hand of a class inherited from QObject.

    From the abstract class SerialPortEngine, it inherits all virtual
    interface methods that are common to all serial ports on any platform.
    These methods, the class WinSerialPortEngine implements on the OS
    Windows platform, using a corresponding Win API.

    From QObject-like class, it inherits a specific system Qt features.
    For example, for NT-based platforms WinSerialPortEngine uses private
    Qt class QWinEventNotifier. Thanks to this class, it have the
    opportunity to asynchronously track the events from the serial port,
    such as the appearance of a character in the receive buffer,
    error I/O, and etc. Ie events are handled in Qt core in its event
    loop, so no need to create additional threads to perform these
    operations. However, for embedded systems, this approach does not work,
    because they have a another Win API. In this case, WinSerialPortEngine
    is derived from QThread and creates an additional thread to keep track
    of events.

    That is, as seen from the above, the functional WinSerialPortEngine
    completely covers all the necessary tasks.
*/

#include "serialportengine_win_p.h"

#include <QtCore/qregexp.h>
#if !defined (Q_OS_WINCE)
#  include <QtCore/qcoreevent.h>
#endif

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

/* Public methods */

/*!
    Constructs a WinSerialPortEngine with \a parent and
    initializes all the internal variables of the initial values.

    A pointer \a parent to the object class SerialPortPrivate
    is required for the recursive call some of its methods.
*/
WinSerialPortEngine::WinSerialPortEngine(SerialPortPrivate *d)
    : m_descriptor(INVALID_HANDLE_VALUE)
    , m_flagErrorFromCommEvent(false)
    , m_currentMask(0)
    , m_desiredMask(0)
#if defined (Q_OS_WINCE)
    , m_running(true)
#endif
{
    Q_ASSERT(d);
    dptr = d;
    size_t size = sizeof(DCB);
    ::memset(&m_currentDcb, 0, size);
    ::memset(&m_restoredDcb, 0, size);
    size = sizeof(COMMTIMEOUTS);
    ::memset(&m_currentCommTimeouts, 0, size);
    ::memset(&m_restoredCommTimeouts, 0, size);

#if !defined (Q_OS_WINCE)
    size = sizeof(OVERLAPPED);
    ::memset(&m_ovRead, 0, size);
    ::memset(&m_ovWrite, 0, size);
    ::memset(&m_ovSelect, 0, size);
    ::memset(&m_ovNotify, 0, size);
#endif
}

/*!
    Stops the tracking events of the serial port and
    destructs a WinSerialPortEngine.
*/
WinSerialPortEngine::~WinSerialPortEngine()
{
#if defined (Q_OS_WINCE)
    m_running = false;
    ::SetCommMask(m_descriptor, 0);
    //terminate();
    wait();
#else
    setEnabled(false);
#endif
}

/*!
    Tries to open the handle desired serial port by \a location in the
    given open \a mode. In the process of discovery, always set a
    serial port in non-blocking mode (when the read operation returns
    immediately) and tries to determine its current configuration and
    install them.

    It should be noted the following features that Windows performs
    when using the serial port:
    - support only binary transfers mode
    - always open in exclusive mode

    For Windows NT-based platforms, the serial port is opened in the
    overlapped mode, with flag FILE_FLAG_OVERLAPPED.

    If successful, returns true; otherwise returns false, with the setup a
    error code.
*/
bool WinSerialPortEngine::open(const QString &location, QIODevice::OpenMode mode)
{
    DWORD desiredAccess = 0;
    DWORD shareMode = 0;
    DWORD flagsAndAttributes = 0;
    bool rxflag = false;
    bool txflag = false;

#if !defined (Q_OS_WINCE)
    flagsAndAttributes |= FILE_FLAG_OVERLAPPED;
#endif

    if (mode & QIODevice::ReadOnly) {
        desiredAccess |= GENERIC_READ;
        //shareMode = FILE_SHARE_READ;
        rxflag = true;
    }
    if (mode & QIODevice::WriteOnly) {
        desiredAccess |= GENERIC_WRITE;
        //shareMode = FILE_SHARE_WRITE;
        txflag = true;
    }

    QByteArray filePath = QByteArray((const char *)location.utf16(),
                                     location.size() * 2 + 1);

    // Try opened serial device.
    m_descriptor = ::CreateFile((const wchar_t*)filePath.constData(),
                                desiredAccess, shareMode, 0, OPEN_EXISTING, flagsAndAttributes, 0);

    if (m_descriptor == INVALID_HANDLE_VALUE) {
        switch (::GetLastError()) {
        case ERROR_FILE_NOT_FOUND:
            dptr->setError(SerialPort::NoSuchDeviceError);
            break;
        case ERROR_ACCESS_DENIED:
            dptr->setError(SerialPort::PermissionDeniedError);
            break;
        default:
            dptr->setError(SerialPort::UnknownPortError);
        }
        return false;
    }

    // Save current DCB port settings.
    DWORD confSize = sizeof(DCB);
    if (::GetCommState(m_descriptor, &m_restoredDcb) == 0) {
        dptr->setError(SerialPort::UnknownPortError);
        return false;
    }
    ::memcpy(&m_currentDcb, &m_restoredDcb, confSize);

    // Set other DCB port options.
    m_currentDcb.fBinary = true;
    m_currentDcb.fInX = false;
    m_currentDcb.fOutX = false;
    m_currentDcb.fAbortOnError = false;
    m_currentDcb.fNull = false;
    m_currentDcb.fErrorChar = false;

    // Apply new DCB init settings.
    if (!updateDcb())
        return false;

    // Save current port timeouts.
    confSize = sizeof(COMMTIMEOUTS);
    if (::GetCommTimeouts(m_descriptor, &m_restoredCommTimeouts) == 0) {
        dptr->setError(SerialPort::UnknownPortError);
        return false;
    }
    ::memcpy(&m_currentCommTimeouts, &m_restoredCommTimeouts, confSize);

    // Set new port timeouts.
    ::memset(&m_currentCommTimeouts, 0, confSize);
    m_currentCommTimeouts.ReadIntervalTimeout = MAXDWORD;

    // Apply new port timeouts.
    if (!updateCommTimeouts())
        return false;

#if !defined (Q_OS_WINCE)
    if (!createEvents(rxflag, txflag)) {
        dptr->setError(SerialPort::UnknownPortError);
        return false;
    }
#endif

    detectDefaultSettings();
    return true;
}

/*!
    Closes a serial port handle. Before closing - restore previous
    serial port settings if necessary.
*/
void WinSerialPortEngine::close(const QString &location)
{
    Q_UNUSED(location);

#if !defined (Q_OS_WINCE)
    ::CancelIo(m_descriptor);
#endif

    if (dptr->options.restoreSettingsOnClose) {
        ::SetCommState(m_descriptor, &m_restoredDcb);
        ::SetCommTimeouts(m_descriptor, &m_restoredCommTimeouts);
    }

    ::CloseHandle(m_descriptor);

#if !defined (Q_OS_WINCE)
    closeEvents();
#endif
    m_descriptor = INVALID_HANDLE_VALUE;
}

/*!
    Returns a bitmap state of RS-232 line signals. On error,
    bitmap will be empty (equal zero).

    Win API allows you to receive only the state of signals:
    CTS, DSR, RING, DCD, DTR, RTS. Other signals are not available.
*/
SerialPort::Lines WinSerialPortEngine::lines() const
{
    DWORD modemStat = 0;
    SerialPort::Lines ret = 0;

    if (::GetCommModemStatus(m_descriptor, &modemStat) == 0) {
        // Print error?
        return ret;
    }

    if (modemStat & MS_CTS_ON)
        ret |= SerialPort::Cts;
    if (modemStat & MS_DSR_ON)
        ret |= SerialPort::Dsr;
    if (modemStat & MS_RING_ON)
        ret |= SerialPort::Ri;
    if (modemStat & MS_RLSD_ON)
        ret |= SerialPort::Dcd;

    DWORD bytesReturned = 0;
    if (::DeviceIoControl(m_descriptor, IOCTL_SERIAL_GET_DTRRTS, 0, 0,
                          &modemStat, sizeof(DWORD),
                          &bytesReturned, 0)) {

        if (modemStat & SERIAL_DTR_STATE)
            ret |= SerialPort::Dtr;
        if (modemStat & SERIAL_RTS_STATE)
            ret |= SerialPort::Rts;
    }

    return ret;
}

/*!
    Set DTR signal to state \a set.

    If successful, returns true; otherwise returns false.
*/
bool WinSerialPortEngine::setDtr(bool set)
{
    bool ret = ::EscapeCommFunction(m_descriptor, (set) ? SETDTR : CLRDTR);
    if (!ret) {
        // FIXME: Here need call ::GetLastError()
        // and set error type.
    }
    return ret;
}

/*!
    Set RTS signal to state \a set.

    If successful, returns true; otherwise returns false.
*/
bool WinSerialPortEngine::setRts(bool set)
{
    bool ret = ::EscapeCommFunction(m_descriptor, (set) ? SETRTS : CLRRTS);
    if (!ret) {
        // FIXME: Here need call ::GetLastError()
        // and set error type.
    }
    return ret;
}

/*!
    Flushes the buffers of a specified serial port and
    causes all buffered data to be written to a serial port.

    If successful, returns true; otherwise returns false.
*/
bool WinSerialPortEngine::flush()
{
    bool ret = ::FlushFileBuffers(m_descriptor);
    if (!ret) {
        // FIXME: Here need call ::GetLastError()
        // and set error type.
    }
    return ret;
}

/*!
    Discards all characters from the output or input buffer of
    a specified communications resource. It can also terminate
    pending read or write operations on the resource.

    If successful, returns true; otherwise returns false.
*/
bool WinSerialPortEngine::reset()
{
    DWORD flags = (PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
    bool ret = ::PurgeComm(m_descriptor, flags);
    if (!ret) {
        // FIXME: Here need call ::GetLastError()
        // and set error type.
    }
    return ret;
}

/*!
    Sends a continuous stream of zero bits during a specified
    period of time \a duration in msec.

    If successful, returns true; otherwise returns false.
*/
bool WinSerialPortEngine::sendBreak(int duration)
{
    // FIXME:
    if (setBreak(true)) {
        ::Sleep(duration);
        if (setBreak(false))
            return true;
    }
    return false;
}

/*!
    Restores or suspend character transmission and places the
    transmission line in a nonbreak or break state,
    depending on the parameter \a set.

    If successful, returns true; otherwise returns false.
*/
bool WinSerialPortEngine::setBreak(bool set)
{
    bool ret = (set) ?
                (::SetCommBreak(m_descriptor)) : (::ClearCommBreak(m_descriptor));
    if (!ret) {
        // FIXME: Here need call ::GetLastError()
        // and set error type.
    }
    return ret;
}

enum CommStatQue { CS_IN_QUE, CS_OUT_QUE };
static qint64 get_commstat_que(HANDLE m_descriptor, enum CommStatQue que)
{
    COMSTAT cs;
    ::memset(&cs, 0, sizeof(COMSTAT));
    if (::ClearCommError(m_descriptor, 0, &cs) == 0)
        return -1;
    return qint64((que == CS_IN_QUE) ? (cs.cbInQue) : (cs.cbOutQue));
}

/*!
    Returns the number of bytes received by the serial provider
    but not yet read by a read() operation. Also it clears the
    device's error flag to enable additional input and output
    (I/O) operations.

    If successful, returns true; otherwise returns false.
*/
qint64 WinSerialPortEngine::bytesAvailable() const
{
    return get_commstat_que(m_descriptor, CS_IN_QUE);
}

/*!
    Returns the number of bytes of user data remaining to be
    transmitted for all write operations. This value will be zero
    for a nonoverlapped write (for embedded platform as WinCE).
    Also it clears the device's error flag to enable additional
    input and output (I/O) operations.

    If successful, returns true; otherwise returns false.
*/
qint64 WinSerialPortEngine::bytesToWrite() const
{
    return get_commstat_que(m_descriptor, CS_OUT_QUE);
}

#if !defined (Q_OS_WINCE)
// Clear overlapped structure, but does not affect the event.
static void clear_overlapped(OVERLAPPED *overlapped)
{
    overlapped->Internal = 0;
    overlapped->InternalHigh = 0;
    overlapped->Offset = 0;
    overlapped->OffsetHigh = 0;
}
#endif

/*!
    Read data from serial port. For NT-based platform, process
    data's reading with the waiting wehn of pending I/O
    operations is complete. Maybe this can cause some freezes.

    If successful, returns to the external buffer \a data the
    real number of bytes read, which can be less than the
    requested \a len; otherwise returned -1 with set error code.

    Also, this method processed the policy of operating with the
    received symbol, in which the parity or frame error is detected.
    This analysis and processing is executed by software-way in
    this method. Parity or frame error flag determines subsystem
    notification when it receives an event type EV_ERR. Since the
    EV_ERR event appears before the event EV_RXCHAR, therefore,
    we are able to handle errors by ordered, for each bad charachter
    in this read method. This is true only when enabled the internal
    read buffer of class SerialPort, ie when it is automatically
    filled when the notification mode of reading is enabled. In
    other cases, policy processing bad char is not guaranteed.
*/
qint64 WinSerialPortEngine::read(char *data, qint64 len)
{
#if !defined (Q_OS_WINCE)
    clear_overlapped(&m_ovRead);
#endif

    DWORD readBytes = 0;
    bool sucessResult = false;

    // FIXME:
    if (dptr->options.policy != SerialPort::IgnorePolicy)
        len = 1;

#if defined (Q_OS_WINCE)
    sucessResult = ::ReadFile(m_descriptor, data, len, &readBytes, 0);
#else
    if (::ReadFile(m_descriptor, data, len, &readBytes, &m_ovRead))
        sucessResult = true;
    else if ((::GetLastError() == ERROR_IO_PENDING)
             // Here have to wait for completion of pending transactions
             // to get the number of actually readed bytes.
             && ::GetOverlappedResult(m_descriptor, &m_ovRead, &readBytes, true)) {

        sucessResult = true;
    }
#endif

    if (!sucessResult) {
        dptr->setError(SerialPort::IoError);
        return -1;
    }

    // FIXME: Process emulate policy.
    if (m_flagErrorFromCommEvent) {
        m_flagErrorFromCommEvent = false;

        switch (dptr->options.policy) {
        case SerialPort::SkipPolicy:
            return 0;
        case SerialPort::PassZeroPolicy:
            *data = '\0';
            break;
        case SerialPort::StopReceivingPolicy:
            break;
        default:;
        }
    }
    return qint64(readBytes);
}

/*!
    Write \a data to serial port. For NT-based platform, process
    data's writing with the waiting wehn of pending I/O
    operations is complete. Maybe this can cause some freezes.

    If successful, returns the real number of bytes write, which
    can be less than the requested \a len; otherwise returned -1
    with set error code.
*/
qint64 WinSerialPortEngine::write(const char *data, qint64 len)
{
#if !defined (Q_OS_WINCE)
    clear_overlapped(&m_ovWrite);
#endif

    DWORD writeBytes = 0;
    bool sucessResult = false;

#if defined (Q_OS_WINCE)
    sucessResult = ::WriteFile(m_descriptor, data, len, &writeBytes, 0);
#else
    if (::WriteFile(m_descriptor, data, len, &writeBytes, &m_ovWrite))
        sucessResult = true;
    else if (::GetLastError() == ERROR_IO_PENDING) {
        // This is not an error. In this case, the number of bytes actually
        // transmitted can be received only after the completion of pending
        // transactions, but it will freeze the event loop. The solution is
        // to fake return results without waiting. Such a maneuver is possible
        // due to the peculiarities of the serial port driver for Windows.
        sucessResult = true;
        writeBytes = len;
    }
#endif

    if (!sucessResult) {
        dptr->setError(SerialPort::IoError);
        return -1;
    }
    return quint64(writeBytes);
}

/*!
    Implements a function blocking for waiting of events EV_RXCHAR or
    EV_TXEMPTY, on the \a timeout in millisecond. Event EV_RXCHAR
    controlled, if the flag \a checkRead is set on true, and
    EV_TXEMPTY wehn flag \a checkWrite is set on true. The result
    of catch in each of the events, save to the corresponding
    variables \a selectForRead and \a selectForWrite.

    For NT-based OS and embedded, this method have different
    implementation. WinCE has no mechanism to exit out of a timeout,
    therefore for this feature special class is used
    WinCeWaitCommEventBreaker, without which it is locked to wait
    forever in the absence of events EV_RXCHAR or EV_TXEMPTY. For
    satisfactory operation of the breaker, the timeout should be
    guaranteed a great, to the timer in the breaker does not trip
    happen sooner than a function call WaitCommEvent(); otherwise it
    will block forever (in the absence of events EV_RXCHAR or EV_TXEMPTY).

    Returns true if the occurrence of any event before the timeout;
    otherwise returns false.
*/
bool WinSerialPortEngine::select(int timeout,
                                 bool checkRead, bool checkWrite,
                                 bool *selectForRead, bool *selectForWrite)
{
    // FIXME: Forward checking available data for read.
    // This is a bad decision, because call bytesAvailable() automatically
    // clears the error parity, frame, etc. That is, then in the future,
    // it is impossible to identify them in the process of reading the data.
    if (checkRead && (bytesAvailable() > 0)) {
        Q_ASSERT(selectForRead);
        *selectForRead = true;
        return true;
    }

#if !defined (Q_OS_WINCE)
    clear_overlapped(&m_ovSelect);
#endif

    DWORD oldEventMask = 0;
    DWORD currEventMask = 0;

    if (checkRead)
        currEventMask |= EV_RXCHAR;
    if (checkWrite)
        currEventMask |= EV_TXEMPTY;

    // Save old mask.
    if (::GetCommMask(m_descriptor, &oldEventMask) == 0) {
        //Print error?
        return false;
    }

    // Checking the old mask bits as in the current mask.
    // And if these bits are not exists, then add them and set the reting mask.
    if (currEventMask != (oldEventMask & currEventMask)) {
        currEventMask |= oldEventMask;
        if (::SetCommMask(m_descriptor, currEventMask) == 0) {
            //Print error?
            return false;
        }
    }

    currEventMask = 0;
    bool sucessResult = false;

#if !defined (Q_OS_WINCE)
    if (::WaitCommEvent(m_descriptor, &currEventMask, &m_ovSelect))
        sucessResult = true;
    else if (::GetLastError() == ERROR_IO_PENDING) {
        DWORD bytesTransferred = 0;
        if ((::WaitForSingleObject(m_ovSelect.hEvent, (timeout < 0) ? 0 : timeout) == WAIT_OBJECT_0)
                && ::GetOverlappedResult(m_descriptor, &m_ovSelect, &bytesTransferred, false)) {

            sucessResult = true;
        } else {
            // Here there was a timeout or other error.
        }
    }
#else
    // FIXME: Here the situation is not properly handled with zero timeout:
    // breaker can work out before you call a method WaitCommEvent()
    // and so it will loop forever!
    WinCeWaitCommEventBreaker breaker(m_descriptor, (timeout < 0) ? 0 : timeout);
    ::WaitCommEvent(m_descriptor, &currEventMask, 0);
    breaker.stop();
    sucessResult = !breaker.isWorked();
#endif

    if (sucessResult) {
        // FIXME: Here call the bytesAvailable() to protect against false positives
        // WaitForSingleObject(), for example, when manually pulling USB/Serial
        // converter from system, ie when devices are in fact not.
        // While it may be possible to make additional checks - to catch an event EV_ERR,
        // adding (in the code above) extra bits in the mask currEventMask.
        if (checkRead) {
            Q_ASSERT(selectForRead);
            *selectForRead = (currEventMask & EV_RXCHAR) && (bytesAvailable() > 0);
        }
        if (checkWrite) {
            Q_ASSERT(selectForWrite);
            *selectForWrite =  (currEventMask & EV_TXEMPTY);
        }
    }

    // Rerair old mask.
    ::SetCommMask(m_descriptor, oldEventMask);
    return sucessResult;
}

#if !defined (Q_OS_WINCE)
static const QString defaultPathPrefix = QLatin1String("\\\\.\\");
#else
static const QString defaultPathPostfix = QLatin1String(":");
#endif

/*!
    Converts a platform specific \a port name to system location
    and return result.
*/
QString WinSerialPortEngine::toSystemLocation(const QString &port) const
{
    QString ret = port;
#if !defined (Q_OS_WINCE)
    if (!ret.contains(defaultPathPrefix))
        ret.prepend(defaultPathPrefix);
#else
    if (!ret.contains(defaultPathPostfix))
        ret.append(defaultPathPostfix);
#endif
    return ret;
}

/*!
    Converts a platform specific system \a location to port name
    and return result.
*/
QString WinSerialPortEngine::fromSystemLocation(const QString &location) const
{
    QString ret = location;
#if !defined (Q_OS_WINCE)
    if (ret.contains(defaultPathPrefix))
        ret.remove(defaultPathPrefix);
#else
    if (ret.contains(defaultPathPostfix))
        ret.remove(defaultPathPostfix);
#endif
    return ret;
}

/*!
    Set desired \a rate by given direction \a dir.
    However, windows does not support separate directions, so the
    method will return an error.

    If successful, returns true; otherwise returns false, with the setup a
    error code.
*/
bool WinSerialPortEngine::setRate(qint32 rate, SerialPort::Directions dir)
{
    if (dir != SerialPort::AllDirections) {
        dptr->setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }
    m_currentDcb.BaudRate = DWORD(rate);
    return updateDcb();
}

/*!
    Set desired number of data bits \a dataBits in byte. Windows
    native supported all present number of data bits 5, 6, 7, 8.

    If successful, returns true; otherwise returns false, with the setup a
    error code.
*/
bool WinSerialPortEngine::setDataBits(SerialPort::DataBits dataBits)
{
    m_currentDcb.ByteSize = BYTE(dataBits);
    return updateDcb();
}

/*!
    Set desired \a parity control mode. Windows native supported
    all present parity types no parity, space, mark, even, odd.

    If successful, returns true; otherwise returns false, with the setup a
    error code.
*/
bool WinSerialPortEngine::setParity(SerialPort::Parity parity)
{
    m_currentDcb.fParity = true;
    switch (parity) {
    case SerialPort::NoParity:
        m_currentDcb.Parity = NOPARITY;
        m_currentDcb.fParity = false;
        break;
    case SerialPort::SpaceParity:
        m_currentDcb.Parity = SPACEPARITY;
        break;
    case SerialPort::MarkParity:
        m_currentDcb.Parity = MARKPARITY;
        break;
    case SerialPort::EvenParity:
        m_currentDcb.Parity = EVENPARITY;
        break;
    case SerialPort::OddParity:
        m_currentDcb.Parity = ODDPARITY;
        break;
    default:
        dptr->setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }
    return updateDcb();
}

/*!
    Set desired number of stop bits \a stopBits in frame.
    Windows native supported all present number of stop bits
    1, 1.5, 2.

    If successful, returns true; otherwise returns false, with the setup a
    error code.
*/
bool WinSerialPortEngine::setStopBits(SerialPort::StopBits stopBits)
{
    switch (stopBits) {
    case SerialPort::OneStop:
        m_currentDcb.StopBits = ONESTOPBIT;
        break;
    case SerialPort::OneAndHalfStop:
        m_currentDcb.StopBits = ONE5STOPBITS;
        break;
    case SerialPort::TwoStop:
        m_currentDcb.StopBits = TWOSTOPBITS;
        break;
    default:
        dptr->setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }
    return updateDcb();
}

/*!
    Set desired \a flow control mode. Windows native supported all
    present flow control modes no control, hardware (RTS/CTS),
    software (XON/XOFF).

    If successful, returns true; otherwise returns false, with the setup a
    error code.
*/
bool WinSerialPortEngine::setFlowControl(SerialPort::FlowControl flow)
{
    switch (flow) {
    case SerialPort::NoFlowControl:
        m_currentDcb.fOutxCtsFlow = false;
        m_currentDcb.fRtsControl = RTS_CONTROL_DISABLE;
        m_currentDcb.fInX = m_currentDcb.fOutX = false;
        break;
    case SerialPort::SoftwareControl:
        m_currentDcb.fOutxCtsFlow = false;
        m_currentDcb.fRtsControl = RTS_CONTROL_DISABLE;
        m_currentDcb.fInX = m_currentDcb.fOutX = true;
        break;
    case SerialPort::HardwareControl:
        m_currentDcb.fOutxCtsFlow = true;
        m_currentDcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
        m_currentDcb.fInX = m_currentDcb.fOutX = false;
        break;
    default:
        dptr->setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }
    return updateDcb();
}

/*!
    Empty stub. Setting a variable is carried out methods in a
    private class SerialPortPrivate.
*/
bool WinSerialPortEngine::setDataErrorPolicy(SerialPort::DataErrorPolicy policy)
{
    Q_UNUSED(policy)
    return true;
}

/*!
    Returns the current status of the read notification subsystem.
*/
bool WinSerialPortEngine::isReadNotificationEnabled() const
{
    return isNotificationEnabled(EV_RXCHAR);
}

/*!
    Enables or disables read notification subsystem, depending on
    the \a enable parameter. If the subsystem is enabled, it will
    asynchronously track the occurrence of an event EV_RXCHAR.
    Thanks to that, SerialPort can emit a signal readyRead() and
    fill up the internal receive buffer with new data, that
    automatically received from a serial port in the event loop.
*/
void WinSerialPortEngine::setReadNotificationEnabled(bool enable)
{
    setNotificationEnabled(enable, EV_RXCHAR);
}

/*!
    Returns the current status of the write notification subsystem.
*/
bool WinSerialPortEngine::isWriteNotificationEnabled() const
{
    return isNotificationEnabled(EV_TXEMPTY);
}

/*!
    Enables or disables write notification subsystem, depending on
    the \a enable parameter. If the subsystem is enabled, it will
    asynchronously track the occurrence of an event EV_TXEMPTY.
    Thanks to that, SerialPort can write data from internal transfer
    buffer, to serial port automatically in the event loop.
*/
void WinSerialPortEngine::setWriteNotificationEnabled(bool enable)
{
    setNotificationEnabled(enable, EV_TXEMPTY);

    // This only for OS Windows, as EV_TXEMPTY event is triggered only
    // after the last byte of data.
    // Therefore, we are forced to run writeNotification(), as EV_TXEMPTY does not work.
    if (enable)
        dptr->canWriteNotification();
}

/*!
    Returns the current status of the errors notification subsystem.
*/
bool WinSerialPortEngine::isErrorNotificationEnabled() const
{
    return isNotificationEnabled(EV_ERR);
}

/*!
    Enables or disables errors notification subsystem, depending on
    the \a enable parameter. If the subsystem is enabled, it will
    asynchronously track the occurrence of an event EV_ERR.
*/
void WinSerialPortEngine::setErrorNotificationEnabled(bool enable)
{
    setNotificationEnabled(enable, EV_ERR);
}

/*!
    Defines the type of parity or frame error when an event
    occurs EV_ERR. In addition, in case of any errors, this method
    sets to true a flag m_flagErrorFromCommEvent, that used in the
    method of reading for policy processing.

    This method is called automatically from an error handler in
    parent class SerialPortPrivate, that called by error notification
    subsystem, wehn occurred a event EV_ERR.

*/
bool WinSerialPortEngine::processIOErrors()
{
    DWORD err = 0;
    bool ret = (::ClearCommError(m_descriptor, &err, 0) != 0);
    if (ret && err) {
        if (err & CE_FRAME)
            dptr->setError(SerialPort::FramingError);
        else if (err & CE_RXPARITY)
            dptr->setError(SerialPort::ParityError);
        else if (err & CE_BREAK)
            dptr->setError(SerialPort::BreakConditionError);
        else
            dptr->setError(SerialPort::UnknownPortError);

        m_flagErrorFromCommEvent = true;
    }
    return ret;
}

#if defined (Q_OS_WINCE)

void WinSerialPortEngine::lockNotification(NotificationLockerType type, bool uselocker)
{
    QMutex *mutex = 0;
    switch (type) {
    case CanReadLocker:
        mutex = &m_readNotificationMutex;
        break;
    case CanWriteLocker:
        mutex = &m_writeNotificationMutex;
        break;
    case CanErrorLocker:
        mutex = &m_errorNotificationMutex;
        break;
    }

    if (uselocker)
        QMutexLocker locker(mutex);
    else
        mutex->lock();
}

void WinSerialPortEngine::unlockNotification(NotificationLockerType type)
{
    switch (type) {
    case CanReadLocker:
        m_readNotificationMutex.unlock();
        break;
    case CanWriteLocker:
        m_writeNotificationMutex.unlock();
        break;
    case CanErrorLocker:
        m_errorNotificationMutex.unlock();
        break;
    }
}

#endif

/* Public static methods */

// This table contains standard values of baud rates that
// are defined in MSDN and/or in Win SDK file winbase.h
static
const qint32 standardRatesTable[] =
{
    #if defined (CBR_110)
    CBR_110,
    #endif
    #if defined (CBR_300)
    CBR_300,
    #endif
    #if defined (CBR_600)
    CBR_600,
    #endif
    #if defined (CBR_1200)
    CBR_1200,
    #endif
    #if defined (CBR_2400)
    CBR_2400,
    #endif
    #if defined (CBR_4800)
    CBR_4800,
    #endif
    #if defined (CBR_9600)
    CBR_9600,
    #endif
    #if defined (CBR_14400)
    CBR_14400,
    #endif
    #if defined (CBR_19200)
    CBR_19200,
    #endif
    #if defined (CBR_38400)
    CBR_38400,
    #endif
    #if defined (CBR_56000)
    CBR_56000,
    #endif
    #if defined (CBR_57600)
    CBR_57600,
    #endif
    #if defined (CBR_115200)
    CBR_115200,
    #endif
    #if defined (CBR_128000)
    CBR_128000,
    #endif
    #if defined (CBR_256000)
    CBR_256000
    #endif
};

static const qint32 *standardRatesTable_end =
        standardRatesTable + sizeof(standardRatesTable)/sizeof(*standardRatesTable);

/*!
   Returns a list standard values of baud rate that
   are defined in MSDN and/or in Win SDK file winbase.h.
*/
QList<qint32> WinSerialPortEngine::standardRates()
{
   QList<qint32> l;
   for (const qint32 *it = standardRatesTable; it != standardRatesTable_end; ++it)
      l.append(*it);
   return l;
}

/* Protected methods */

/*!
    Attempts to determine the current settings of the serial port,
    wehn it opened. Used only in the method open().
*/
void WinSerialPortEngine::detectDefaultSettings()
{
    // Detect rate.
    dptr->options.inputRate = quint32(m_currentDcb.BaudRate);
    dptr->options.outputRate = dptr->options.inputRate;

    // Detect databits.
    switch (m_currentDcb.ByteSize) {
    case 5:
        dptr->options.dataBits = SerialPort::Data5;
        break;
    case 6:
        dptr->options.dataBits = SerialPort::Data6;
        break;
    case 7:
        dptr->options.dataBits = SerialPort::Data7;
        break;
    case 8:
        dptr->options.dataBits = SerialPort::Data8;
        break;
    default:
        dptr->options.dataBits = SerialPort::UnknownDataBits;
    }

    // Detect parity.
    if ((m_currentDcb.Parity == NOPARITY) && !m_currentDcb.fParity)
        dptr->options.parity = SerialPort::NoParity;
    else if ((m_currentDcb.Parity == SPACEPARITY) && m_currentDcb.fParity)
        dptr->options.parity = SerialPort::SpaceParity;
    else if ((m_currentDcb.Parity == MARKPARITY) && m_currentDcb.fParity)
        dptr->options.parity = SerialPort::MarkParity;
    else if ((m_currentDcb.Parity == EVENPARITY) && m_currentDcb.fParity)
        dptr->options.parity = SerialPort::EvenParity;
    else if ((m_currentDcb.Parity == ODDPARITY) && m_currentDcb.fParity)
        dptr->options.parity = SerialPort::OddParity;
    else
        dptr->options.parity = SerialPort::UnknownParity;

    // Detect stopbits.
    switch (m_currentDcb.StopBits) {
    case ONESTOPBIT:
        dptr->options.stopBits = SerialPort::OneStop;
        break;
    case ONE5STOPBITS:
        dptr->options.stopBits = SerialPort::OneAndHalfStop;
        break;
    case TWOSTOPBITS:
        dptr->options.stopBits = SerialPort::TwoStop;
        break;
    default:
        dptr->options.stopBits = SerialPort::UnknownStopBits;
    }

    // Detect flow control.
    if (!m_currentDcb.fOutxCtsFlow && (m_currentDcb.fRtsControl == RTS_CONTROL_DISABLE)
            && !m_currentDcb.fInX && !m_currentDcb.fOutX) {
        dptr->options.flow = SerialPort::NoFlowControl;
    } else if (!m_currentDcb.fOutxCtsFlow && (m_currentDcb.fRtsControl == RTS_CONTROL_DISABLE)
               && m_currentDcb.fInX && m_currentDcb.fOutX) {
        dptr->options.flow = SerialPort::SoftwareControl;
    } else if (m_currentDcb.fOutxCtsFlow && (m_currentDcb.fRtsControl == RTS_CONTROL_HANDSHAKE)
               && !m_currentDcb.fInX && !m_currentDcb.fOutX) {
        dptr->options.flow = SerialPort::HardwareControl;
    } else
        dptr->options.flow = SerialPort::UnknownFlowControl;
}

#if defined (Q_OS_WINCE)

/*!
    Embedded-based (WinCE) event loop for notification subsystem.
    Tracking a separate thread the events from the serial port, as:
    EV_ERR, EV_RXCHAR, EV_TXEMPTY. When is occur a relevant event,
    calls him handler from a parent class SerialPortPrivate.
    At the same time in handlers to capture/release the mutex
    (see handlers implementation).
*/
void WinSerialPortEngine::run()
{
    while (m_running) {

        m_setCommMaskMutex.lock();
        ::SetCommMask(m_descriptor, m_desiredMask);
        m_setCommMaskMutex.unlock();

        if (::WaitCommEvent(m_descriptor, &m_currentMask, 0) != 0) {

            // Wait until complete the operation changes the port settings,
            // see updateDcb().
            m_settingsChangeMutex.lock();
            m_settingsChangeMutex.unlock();

            if (EV_ERR & m_currentMask & m_desiredMask) {
                dptr->canErrorNotification();
            }
            if (EV_RXCHAR & m_currentMask & m_desiredMask) {
                dptr->canReadNotification();
            }
            //FIXME: This is why it does not work?
            if (EV_TXEMPTY & m_currentMask & m_desiredMask) {
                dptr->canWriteNotification();
            }
        }
    }
}

#else

/*!
    Windows NT-based event loop for notification subsystem.
    Asynchronously in event loop continuous mode tracking the
    events from the serial port, as: EV_ERR, EV_RXCHAR, EV_TXEMPTY.
    When is occur a relevant event, calls him handler from
    a parent class SerialPortPrivate.
*/
bool WinSerialPortEngine::event(QEvent *e)
{
    bool ret = false;
    if (e->type() == QEvent::WinEventAct) {
        if (EV_ERR & m_currentMask & m_desiredMask) {
            dptr->canErrorNotification();
            ret = true;
        }
        if (EV_RXCHAR & m_currentMask & m_desiredMask) {
            dptr->canReadNotification();
            ret = true;
        }
        //FIXME: This is why it does not work?
        if (EV_TXEMPTY & m_currentMask & m_desiredMask) {
            dptr->canWriteNotification();
            ret = true;
        }
    }
    else
        ret = QWinEventNotifier::event(e);

    ::WaitCommEvent(m_descriptor, &m_currentMask, &m_ovNotify);
    return ret;
}

#endif

/* Private methods */

#if !defined (Q_OS_WINCE)

/*!
    For Windows NT-based, creates handles events for OVERLAPPED
    structures, that are used in the methods of reading \a rx,
    writing \a tx, and waiting for data from the serial port.
    This method is only used in the method open().

    If successful, returns true; otherwise returns false.
*/
bool WinSerialPortEngine::createEvents(bool rx, bool tx)
{
    if (rx) {
        m_ovRead.hEvent = ::CreateEvent(0, false, false, 0);
        Q_ASSERT(m_ovRead.hEvent);
    }
    if (tx) {
        m_ovWrite.hEvent = ::CreateEvent(0, false, false, 0);
        Q_ASSERT(m_ovWrite.hEvent);
    }
    m_ovSelect.hEvent = ::CreateEvent(0, false, false, 0);
    Q_ASSERT(m_ovSelect.hEvent);
    m_ovNotify.hEvent = ::CreateEvent(0, false, false, 0);
    Q_ASSERT(m_ovNotify.hEvent);

    setHandle(m_ovNotify.hEvent);
    return true;
}

/*!
    For Windows NT-based, release and closed handles events from
    OVERLAPPED structures.
*/
void WinSerialPortEngine::closeEvents()
{
    if (m_ovRead.hEvent)
        ::CloseHandle(m_ovRead.hEvent);
    if (m_ovWrite.hEvent)
        ::CloseHandle(m_ovWrite.hEvent);
    if (m_ovSelect.hEvent)
        ::CloseHandle(m_ovSelect.hEvent);
    if (m_ovNotify.hEvent)
        ::CloseHandle(m_ovNotify.hEvent);

    size_t size = sizeof(OVERLAPPED);
    ::memset(&m_ovRead, 0, size);
    ::memset(&m_ovWrite, 0, size);
    ::memset(&m_ovSelect, 0, size);
    ::memset(&m_ovNotify, 0, size);
}

#endif

/*!
*/
bool WinSerialPortEngine::isNotificationEnabled(DWORD mask) const
{
    bool enabled;
#if defined (Q_OS_WINCE)
    enabled = isRunning();
#else
    enabled = isEnabled();
#endif
    return (enabled && (m_desiredMask & mask));
}

/*!
*/
void WinSerialPortEngine::setNotificationEnabled(bool enable, DWORD mask)
{

#if defined (Q_OS_WINCE)
    m_setCommMaskMutex.lock();
    ::GetCommMask(m_descriptor, &m_currentMask);
#endif

    // Mask only the desired bits without affecting others.
    if (enable)
        m_desiredMask |= mask;
    else
        m_desiredMask &= ~mask;

    ::SetCommMask(m_descriptor, m_desiredMask);

#if defined (Q_OS_WINCE)
    m_setCommMaskMutex.unlock();

    if (enable && !isRunning())
        start();
#else
    enable = isEnabled();

    // If the desired mask is zero then no needed to restart
    // the monitoring events and needed disable notification;
    // otherwise needed start the notification if it was stopped.
    if (m_desiredMask) {
        ::WaitCommEvent(m_descriptor, &m_currentMask, &m_ovNotify);
        if (!enable)
            setEnabled(true);
    } else if (enable) {
        setEnabled(false);
    }
#endif

}

/*!
    Updates the DCB structure wehn changing of any the parameters
    a serial port.

    If successful, returns true; otherwise returns false.
*/
bool WinSerialPortEngine::updateDcb()
{
#if defined (Q_OS_WINCE)
    // Grab a mutex, in order after exit WaitCommEvent
    // block the flow of run() notifier until there is a change DCB.
    QMutexLocker locker(&m_settingsChangeMutex);
    // This way, we reset in class WaitCommEvent to
    // be able to change the DCB.
    // Otherwise WaitCommEvent blocking any change!
    ::SetCommMask(m_descriptor, 0);
#endif
    if (::SetCommState(m_descriptor, &m_currentDcb) == 0) {
        dptr->setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }
    return true;
}

/*!
    Updates the COMMTIMEUTS structure wehn changing of any timeout the
    parameters a serial port.

    If successful, returns true; otherwise returns false.
*/
bool WinSerialPortEngine::updateCommTimeouts()
{
    if (::SetCommTimeouts(m_descriptor, &m_currentCommTimeouts) == 0) {
        dptr->setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }
    return true;
}

// From <serialportengine_p.h>
SerialPortEngine *SerialPortEngine::create(SerialPortPrivate *d)
{
    return new WinSerialPortEngine(d);
}

#include "moc_serialportengine_win_p.cpp"

QT_END_NAMESPACE_SERIALPORT

