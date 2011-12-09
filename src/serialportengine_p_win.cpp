/*
    License...
*/

/*!
    \class WinSerialPortEngine
    \internal

    \brief The WinSerialPortEngine class provides windows OS
    platform-specific low level access to a serial port.

    \reentrant
    \ingroup serial
    \inmodule QSerialDevice

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

#include "serialportengine_p_win.h"

#include <QtCore/qregexp.h>
#if !defined (Q_OS_WINCE)
#  include <QtCore/qcoreevent.h>
#endif
//#include <QtCore/QDebug>

#ifndef Q_CC_MSVC
#  include <ddk/ntddser.h>
#else

#  ifndef CTL_CODE
#    define CTL_CODE(DeviceType, Function, Method, Access) ( \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
    )
#  endif

#  ifndef FILE_DEVICE_SERIAL_PORT
#    define FILE_DEVICE_SERIAL_PORT	27
#  endif

#  ifndef METHOD_BUFFERED
#    define METHOD_BUFFERED  0
#  endif

#  ifndef FILE_ANY_ACCESS
#    define FILE_ANY_ACCESS  0x00000000
#  endif

#  ifndef IOCTL_SERIAL_GET_DTRRTS
#    define IOCTL_SERIAL_GET_DTRRTS \
    CTL_CODE (FILE_DEVICE_SERIAL_PORT, 30, METHOD_BUFFERED, FILE_ANY_ACCESS)
#  endif

#  ifndef SERIAL_DTR_STATE
#    define SERIAL_DTR_STATE  0x00000001
#  endif

#  ifndef SERIAL_RTS_STATE
#    define SERIAL_RTS_STATE  0x00000002
#  endif

#endif

QT_USE_NAMESPACE

/* Public methods */

/*!
    Constructs a WinSerialPortEngine with \a parent and
    initializes all the internal variables of the initial values.

    A pointer \a parent to the object class SerialPortPrivate
    is required for the recursive call some of its methods.
*/
WinSerialPortEngine::WinSerialPortEngine(SerialPortPrivate *parent)
    : m_descriptor(INVALID_HANDLE_VALUE)
    , m_flagErrorFromCommEvent(false)
    , m_currentMask(0)
    , m_setMask(EV_ERR)
    #if defined (Q_OS_WINCE)
    , m_running(true)
    #endif
{
    Q_ASSERT(parent);
    m_parent = parent;
    size_t size = sizeof(DCB);
    ::memset(&m_currDCB, 0, size);
    ::memset(&m_oldDCB, 0, size);
    size = sizeof(COMMTIMEOUTS);
    ::memset(&m_currCommTimeouts, 0, size);
    ::memset(&m_oldCommTimeouts, 0, size);

#if !defined (Q_OS_WINCE)
    size = sizeof(OVERLAPPED);
    ::memset(&m_ovRead, 0, size);
    ::memset(&m_ovWrite, 0, size);
    ::memset(&m_ovSelect, 0, size);
    ::memset(&m_ov, 0, size);
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

    If successful, returns true; otherwise false, with the setup a
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
            m_parent->setError(SerialPort::NoSuchDeviceError);
            break;
        case ERROR_ACCESS_DENIED:
            m_parent->setError(SerialPort::PermissionDeniedError);
            break;
        default:
            m_parent->setError(SerialPort::UnknownPortError);
        }
        return false;
    }

    // Save current DCB port settings.
    DWORD confSize = sizeof(DCB);
    if (::GetCommState(m_descriptor, &m_oldDCB) == 0) {
        m_parent->setError(SerialPort::UnknownPortError);
        return false;
    }
    ::memcpy(&m_currDCB, &m_oldDCB, confSize);

    // Set other DCB port options.
    m_currDCB.fBinary = true;
    m_currDCB.fInX = false;
    m_currDCB.fOutX = false;
    m_currDCB.fAbortOnError = false;
    m_currDCB.fNull = false;
    m_currDCB.fErrorChar = false;

    // Apply new DCB init settings.
    if (!updateDcb())
        return false;

    // Save current port timeouts.
    confSize = sizeof(COMMTIMEOUTS);
    if (::GetCommTimeouts(m_descriptor, &m_oldCommTimeouts) == 0) {
        m_parent->setError(SerialPort::UnknownPortError);
        return false;
    }
    ::memcpy(&m_currCommTimeouts, &m_oldCommTimeouts, confSize);

    // Set new port timeouts.
    ::memset(&m_currCommTimeouts, 0, confSize);
    m_currCommTimeouts.ReadIntervalTimeout = MAXDWORD;

    // Apply new port timeouts.
    if (!updateCommTimeouts())
        return false;

#if !defined (Q_OS_WINCE)
    if (!createEvents(rxflag, txflag)) {
        m_parent->setError(SerialPort::UnknownPortError);
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

    if (m_parent->m_restoreSettingsOnClose) {
        ::SetCommState(m_descriptor, &m_oldDCB);
        ::SetCommTimeouts(m_descriptor, &m_oldCommTimeouts);
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

    If successful, returns true; otherwise false.
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

    If successful, returns true; otherwise false.
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

    If successful, returns true; otherwise false.
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

    If successful, returns true; otherwise false.
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

    If successful, returns true; otherwise false.
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

    If successful, returns true; otherwise false.
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
static qint64 get_commstat_que(HANDLE descriptor, enum CommStatQue que)
{
    DWORD err;
    COMSTAT cs;
    if (::ClearCommError(descriptor, &err, &cs) == 0)
        return -1;
    return qint64((que == CS_IN_QUE) ? (cs.cbInQue) : (cs.cbOutQue));
}

/*!
    Returns the number of bytes received by the serial provider
    but not yet read by a read() operation. Also it clears the
    device's error flag to enable additional input and output
    (I/O) operations.

    If successful, returns true; otherwise false.
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

    If successful, returns true; otherwise false.
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
    if (m_parent->m_policy != SerialPort::IgnorePolicy)
        len = 1;

#if defined (Q_OS_WINCE)
    sucessResult = ::ReadFile(m_descriptor, data, len, &readBytes, 0);
#else
    if (::ReadFile(m_descriptor, data, len, &readBytes, &m_ovRead))
        sucessResult = true;
    else {
        if (::GetLastError() == ERROR_IO_PENDING) {
            // FIXME: Instead of an infinite wait I/O (not looped), we expect, for example 5 seconds.
            // Although, maybe there is a better solution.
            switch (::WaitForSingleObject(m_ovRead.hEvent, 5000)) {
            case WAIT_OBJECT_0:
                if (::GetOverlappedResult(m_descriptor, &m_ovRead, &readBytes, false))
                    sucessResult = true;
                break;
            default: ;
            }
        }
    }
#endif

    if(!sucessResult) {
        m_parent->setError(SerialPort::IoError);
        return -1;
    }

    // FIXME: Process emulate policy.
    if (m_flagErrorFromCommEvent) {
        m_flagErrorFromCommEvent = false;

        switch (m_parent->m_policy) {
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
    else {
        if (::GetLastError() == ERROR_IO_PENDING) {
            // Instead of an infinite wait I/O (not looped), we expect, for example 5 seconds.
            // Although, maybe there is a better solution.
            switch (::WaitForSingleObject(m_ovWrite.hEvent, 5000)) {
            case WAIT_OBJECT_0:
                if (::GetOverlappedResult(m_descriptor, &m_ovWrite, &writeBytes, false))
                    sucessResult = true;
                break;
            default: ;
            }
        }
    }
#endif

    if(!sucessResult) {
        m_parent->setError(SerialPort::IoError);
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
    // Forward checking data for read.
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
    else {
        if (::GetLastError() == ERROR_IO_PENDING) {
            DWORD bytesTransferred = 0;
            switch (::WaitForSingleObject(m_ovSelect.hEvent, (timeout < 0) ? 0 : timeout)) {
            case WAIT_OBJECT_0:
                if (::GetOverlappedResult(m_descriptor, &m_ovSelect, &bytesTransferred, false))
                    sucessResult = true;
                break;
            default: ;
            }
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
        // Here call the bytesAvailable() to protect against false positives WaitForSingleObject(),
        // for example, when manually pulling USB/Serial converter from system,
        // ie when devices are in fact not.
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
static const QString defaultPathPrefix = "\\\\.\\";
#else
static const QString defaultPathPostfix = ":";
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

    If successful, returns true; otherwise false, with the setup a
    error code.
*/
bool WinSerialPortEngine::setRate(qint32 rate, SerialPort::Directions dir)
{
    if (dir != SerialPort::AllDirections) {
        m_parent->setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }
    m_currDCB.BaudRate = DWORD(rate);
    return updateDcb();
}

/*!
    Set desired number of data bits \a dataBits in byte. Windows
    native supported all present number of data bits 5, 6, 7, 8.

    If successful, returns true; otherwise false, with the setup a
    error code.
*/
bool WinSerialPortEngine::setDataBits(SerialPort::DataBits dataBits)
{
    m_currDCB.ByteSize = BYTE(dataBits);
    return updateDcb();
}

/*!
    Set desired \a parity control mode. Windows native supported
    all present parity types no parity, space, mark, even, odd.

    If successful, returns true; otherwise false, with the setup a
    error code.
*/
bool WinSerialPortEngine::setParity(SerialPort::Parity parity)
{
    m_currDCB.fParity = true;
    switch (parity) {
    case SerialPort::NoParity:
        m_currDCB.Parity = NOPARITY;
        m_currDCB.fParity = false;
        break;
    case SerialPort::SpaceParity:
        m_currDCB.Parity = SPACEPARITY;
        break;
    case SerialPort::MarkParity:
        m_currDCB.Parity = MARKPARITY;
        break;
    case SerialPort::EvenParity:
        m_currDCB.Parity = EVENPARITY;
        break;
    case SerialPort::OddParity:
        m_currDCB.Parity = ODDPARITY;
        break;
    default:
        m_parent->setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }
    return updateDcb();
}

/*!
    Set desired number of stop bits \a stopBits in frame.
    Windows native supported all present number of stop bits
    1, 1.5, 2.

    If successful, returns true; otherwise false, with the setup a
    error code.
*/
bool WinSerialPortEngine::setStopBits(SerialPort::StopBits stopBits)
{
    switch (stopBits) {
    case SerialPort::OneStop:
        m_currDCB.StopBits = ONESTOPBIT;
        break;
    case SerialPort::OneAndHalfStop:
        m_currDCB.StopBits = ONE5STOPBITS;
        break;
    case SerialPort::TwoStop:
        m_currDCB.StopBits = TWOSTOPBITS;
        break;
    default:
        m_parent->setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }
    return updateDcb();
}

/*!
    Set desired \a flow control mode. Windows native supported all
    present flow control modes no control, hardware (RTS/CTS),
    software (XON/XOFF).

    If successful, returns true; otherwise false, with the setup a
    error code.
*/
bool WinSerialPortEngine::setFlowControl(SerialPort::FlowControl flow)
{
    switch (flow) {
    case SerialPort::NoFlowControl:
        m_currDCB.fOutxCtsFlow = false;
        m_currDCB.fRtsControl = RTS_CONTROL_DISABLE;
        m_currDCB.fInX = m_currDCB.fOutX = false;
        break;
    case SerialPort::SoftwareControl:
        m_currDCB.fOutxCtsFlow = false;
        m_currDCB.fRtsControl = RTS_CONTROL_DISABLE;
        m_currDCB.fInX = m_currDCB.fOutX = true;
        break;
    case SerialPort::HardwareControl:
        m_currDCB.fOutxCtsFlow = true;
        m_currDCB.fRtsControl = RTS_CONTROL_HANDSHAKE;
        m_currDCB.fInX = m_currDCB.fOutX = false;
        break;
    default:
        m_parent->setError(SerialPort::UnsupportedPortOperationError);
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
#if defined (Q_OS_WINCE)
    bool flag = isRunning();
#else
    bool flag = isEnabled();
#endif
    return (flag && (m_setMask & EV_RXCHAR));
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
#if defined (Q_OS_WINCE)
    m_setCommMaskMutex.lock();
    ::GetCommMask(m_descriptor, &m_currentMask);
#endif

    if (enable)
        m_setMask |= EV_RXCHAR;
    else
        m_setMask &= ~EV_RXCHAR;

#if defined (Q_OS_WINCE)
    if (m_setMask != m_currentMask)
        ::SetCommMask(m_descriptor, m_setMask);

    m_setCommMaskMutex.unlock();

    if (enable && !isRunning())
        start();
#else
    setMaskAndActivateEvent();
#endif
}

/*!
    Returns the current status of the write notification subsystem.
*/
bool WinSerialPortEngine::isWriteNotificationEnabled() const
{
#if defined (Q_OS_WINCE)
    bool flag = isRunning();
#else
    bool flag = isEnabled();
#endif
    return (flag && (m_setMask & EV_TXEMPTY));
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
#if defined (Q_OS_WINCE)
    m_setCommMaskMutex.lock();
    ::GetCommMask(m_descriptor, &m_currentMask);
#endif

    if (enable)
        m_setMask |= EV_TXEMPTY;
    else
        m_setMask &= ~EV_TXEMPTY;

#if defined (Q_OS_WINCE)
    if (m_setMask != m_currentMask)
        ::SetCommMask(m_descriptor, m_setMask);

    m_setCommMaskMutex.unlock();

    if (enable && !isRunning())
        start();
#else
    setMaskAndActivateEvent();
#endif
    // This only for OS Windows, as EV_TXEMPTY event is triggered only
    // after the last byte of data.
    // Therefore, we are forced to run writeNotification(), as EV_TXEMPTY does not work.
    if (enable)
        m_parent->canWriteNotification();
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
    COMSTAT cs;
    bool ret = (::ClearCommError(m_descriptor, &err, &cs) != 0);
    if (ret && err) {
        if (err & CE_FRAME)
            m_parent->setError(SerialPort::FramingError);
        else if (err & CE_RXPARITY)
            m_parent->setError(SerialPort::ParityError);
        else if (err & CE_BREAK)
            m_parent->setError(SerialPort::BreakConditionError);
        else
            m_parent->setError(SerialPort::UnknownPortError);

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
    case CanReadLocker: m_readNotificationMutex.unlock();
        break;
    case CanWriteLocker: m_writeNotificationMutex.unlock();
        break;
    case CanErrorLocker: m_errorNotificationMutex.unlock();
        break;
    }
}

#endif

/* Protected methods */

/*!
    Attempts to determine the current settings of the serial port,
    wehn it opened. Used only in the method open().
*/
void WinSerialPortEngine::detectDefaultSettings()
{
    // Detect rate.
    m_parent->m_inRate = quint32(m_currDCB.BaudRate);
    m_parent->m_outRate = m_parent->m_inRate;

    // Detect databits.
    switch (m_currDCB.ByteSize) {
    case 5:
        m_parent->m_dataBits = SerialPort::Data5;
        break;
    case 6:
        m_parent->m_dataBits = SerialPort::Data6;
        break;
    case 7:
        m_parent->m_dataBits = SerialPort::Data7;
        break;
    case 8:
        m_parent->m_dataBits = SerialPort::Data8;
        break;
    default:
        m_parent->m_dataBits = SerialPort::UnknownDataBits;
    }

    // Detect parity.
    if ((m_currDCB.Parity == NOPARITY) && !m_currDCB.fParity)
        m_parent->m_parity = SerialPort::NoParity;
    else if ((m_currDCB.Parity == SPACEPARITY) && m_currDCB.fParity)
        m_parent->m_parity = SerialPort::SpaceParity;
    else if ((m_currDCB.Parity == MARKPARITY) && m_currDCB.fParity)
        m_parent->m_parity = SerialPort::MarkParity;
    else if ((m_currDCB.Parity == EVENPARITY) && m_currDCB.fParity)
        m_parent->m_parity = SerialPort::EvenParity;
    else if ((m_currDCB.Parity == ODDPARITY) && m_currDCB.fParity)
        m_parent->m_parity = SerialPort::OddParity;
    else
        m_parent->m_parity = SerialPort::UnknownParity;

    // Detect stopbits.
    switch (m_currDCB.StopBits) {
    case ONESTOPBIT:
        m_parent->m_stopBits = SerialPort::OneStop;
        break;
    case ONE5STOPBITS:
        m_parent->m_stopBits = SerialPort::OneAndHalfStop;
        break;
    case TWOSTOPBITS:
        m_parent->m_stopBits = SerialPort::TwoStop;
        break;
    default:
        m_parent->m_stopBits = SerialPort::UnknownStopBits;
    }

    // Detect flow control.
    if (!m_currDCB.fOutxCtsFlow && (m_currDCB.fRtsControl == RTS_CONTROL_DISABLE)
            && !m_currDCB.fInX && !m_currDCB.fOutX) {
        m_parent->m_flow = SerialPort::NoFlowControl;
    } else if (!m_currDCB.fOutxCtsFlow && (m_currDCB.fRtsControl == RTS_CONTROL_DISABLE)
               && m_currDCB.fInX && m_currDCB.fOutX) {
        m_parent->m_flow = SerialPort::SoftwareControl;
    } else if (m_currDCB.fOutxCtsFlow && (m_currDCB.fRtsControl == RTS_CONTROL_HANDSHAKE)
               && !m_currDCB.fInX && !m_currDCB.fOutX) {
        m_parent->m_flow = SerialPort::HardwareControl;
    } else
        m_parent->m_flow = SerialPort::UnknownFlowControl;
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
        ::SetCommMask(m_descriptor, m_setMask);
        m_setCommMaskMutex.unlock();

        if (::WaitCommEvent(m_descriptor, &m_currentMask, 0) != 0) {

            // Wait until complete the operation changes the port settings,
            // see updateDcb().
            m_settingsChangeMutex.lock();
            m_settingsChangeMutex.unlock();

            if (EV_ERR & m_currentMask & m_setMask) {
                m_parent->canErrorNotification();
            }
            if (EV_RXCHAR & m_currentMask & m_setMask) {
                m_parent->canReadNotification();
            }
            //FIXME: This is why it does not work?
            if (EV_TXEMPTY & m_currentMask & m_setMask) {
                m_parent->canWriteNotification();
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
        if (EV_ERR & m_currentMask & m_setMask) {
            m_parent->canErrorNotification();
            ret = true;
        }
        if (EV_RXCHAR & m_currentMask & m_setMask) {
            m_parent->canReadNotification();
            ret = true;
        }
        //FIXME: This is why it does not work?
        if (EV_TXEMPTY & m_currentMask & m_setMask) {
            m_parent->canWriteNotification();
            ret = true;
        }
    }
    else
        ret = QWinEventNotifier::event(e);

    ::WaitCommEvent(m_descriptor, &m_currentMask, &m_ov);
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

    If successful, returns true; otherwise false.
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
    m_ov.hEvent = ::CreateEvent(0, false, false, 0);
    Q_ASSERT(m_ov.hEvent);

    setHandle(m_ov.hEvent);
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
    if (m_ov.hEvent)
        ::CloseHandle(m_ov.hEvent);

    size_t size = sizeof(OVERLAPPED);
    ::memset(&m_ovRead, 0, size);
    ::memset(&m_ovWrite, 0, size);
    ::memset(&m_ovSelect, 0, size);
    ::memset(&m_ov, 0, size);
}

/*!
    For Windows NT-based, sets the mask of tracking events.
*/
void WinSerialPortEngine::setMaskAndActivateEvent()
{
    ::SetCommMask(m_descriptor, m_setMask);
    if (m_setMask)
        ::WaitCommEvent(m_descriptor, &m_currentMask, &m_ov);
    switch (m_setMask) {
    case 0:
        if (isEnabled())
            setEnabled(false);
        break;
    default:
        if (!isEnabled())
            setEnabled(true);
    }
}

#endif

/*!
    Updates the DCB structure wehn changing of any the parameters
    a serial port.

    If successful, returns true; otherwise false.
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
    if (::SetCommState(m_descriptor, &m_currDCB) == 0) {
        m_parent->setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }
    return true;
}

/*!
    Updates the COMMTIMEUTS structure wehn changing of any timeout the
    parameters a serial port.

    If successful, returns true; otherwise false.
*/
bool WinSerialPortEngine::updateCommTimeouts()
{
    if (::SetCommTimeouts(m_descriptor, &m_currCommTimeouts) == 0) {
        m_parent->setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }
    return true;
}

// From <serialportengine_p.h>
SerialPortEngine *SerialPortEngine::create(SerialPortPrivate *parent)
{
    return new WinSerialPortEngine(parent);
}

