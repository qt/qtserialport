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

/*!
    \class UnixSerialPortEngine
    \internal

    \brief The UnixSerialPortEngine class provides *nix OS
    platform-specific low level access to a serial port.

    \reentrant
    \ingroup serialport-main
    \inmodule QtSerialPort

    Currently the class supports all POSIX-compatible OS (GNU/Linux, *BSD,
    Mac OSX and so forth).

    UnixSerialPortEngine (as well as other platform-dependent engines)
    is a class with multiple inheritance, which on the one hand,
    derives from a general abstract class interface SerialPortEngine,
    on the other hand from QObject.

    From the abstract class SerialPortEngine, this class inherits all virtual
    interface methods that are common to all serial ports on any platform. The
    class UnixSerialPortEngine implements these methods using the POSIX API.

    From QObject, this class inherits system specific Qt features. For example,
    to track serial port events, this class uses the virtual QObject method
    eventFilter(), which analyses the events from the type class
    QSocketNotifier.
*/

#include "serialportengine_unix_p.h"
#include "ttylocker_unix_p.h"

#include <errno.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#if defined (Q_OS_MAC)
#  if defined (MAC_OS_X_VERSION_10_4) && (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4)
#    include <IOKit/serial/ioss.h>
#  endif
#endif

#include <QtCore/qdebug.h>
#include <QtCore/qsocketnotifier.h>
#include <QtCore/qcoreevent.h>

QT_BEGIN_NAMESPACE_SERIALPORT

/* Public methods */

/*!
    Constructs a UnixSerialPortEngine and initializes all internal variables
    to their initial values. The pointer \a d to the private object of class
    SerialPortPrivate is used to call some common methods.
*/
UnixSerialPortEngine::UnixSerialPortEngine(SerialPortPrivate *d)
    : m_descriptor(-1)
    , m_isCustomRateSupported(false)
    , m_readNotifier(0)
    , m_writeNotifier(0)
    , m_exceptionNotifier(0)
{
    Q_ASSERT(d);
    dptr = d;
    int size = sizeof(struct termios);
    ::memset(&m_currentTermios, 0, size);
    ::memset(&m_restoredTermios, 0, size);
}

/*!
    Stops the serial port event tracking and destructs a UnixSerialPortEngine.
*/
UnixSerialPortEngine::~UnixSerialPortEngine()
{
    if (m_readNotifier)
        m_readNotifier->setEnabled(false);
    if (m_writeNotifier)
        m_writeNotifier->setEnabled(false);
    if (m_exceptionNotifier)
        m_exceptionNotifier->setEnabled(false);
}

/*!
    Attempts to open the desired serial port by \a location in the given
    open \a mode.

    Before opening the serial port, open() checks if the appropriate lock file
    exists. If the lock file is present and contains valid information, the
    current serial port is already occupied and thus cannot be opened.

    In the process of discovery, always sets the serial port in non-blocking
    mode (where the read operation returns immediately) and tries to determine
    and install the current configuration to the serial port.

    Since the POSIX API by default opens the port in shared mode,
    this method forces the port in exclusive access mode.
    This is done simultaneously in two ways:
    - the flag TIOCEXCL is set to the pre-opened m_descriptor
    - a lock file is created, which contains the PID of the process, that
      opened the port and other information

    Two methods need to be used due to the fact that some platforms doesn't
    support the constant TIOCEXCL. In this case, only the lock file is used.
    Lock file creation and analysis is done by the helper class TTYLocker.

    If successful, returns true; otherwise returns false and sets an
    error code.
*/
bool UnixSerialPortEngine::open(const QString &location, QIODevice::OpenMode mode)
{
    // First, here need check locked device or not.
    bool byCurrPid = false;
    if (TTYLocker::isLocked(location, &byCurrPid)) {
        dptr->setError(SerialPort::PermissionDeniedError);
        return false;
    }

    int flags = (O_NOCTTY | O_NDELAY);

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

    // Try opened serial device.
    m_descriptor = ::open(location.toLocal8Bit().constData(), flags);

    if (m_descriptor == -1) {
        switch (errno) {
        case ENODEV:
            dptr->setError(SerialPort::NoSuchDeviceError);
            break;
        case EACCES:
            dptr->setError(SerialPort::PermissionDeniedError);
            break;
        default:
            dptr->setError(SerialPort::UnknownPortError);
        }
        return false;
    }

    // Try lock device by location and check it state is locked.
    TTYLocker::lock(location);
    if (!TTYLocker::isLocked(location, &byCurrPid)) {
        dptr->setError(SerialPort::PermissionDeniedError);
        return false;
    }

    // Try set exclusive mode.
#if defined (TIOCEXCL)
    ::ioctl(m_descriptor, TIOCEXCL);
#endif

    // Save current port settings.
    if (::tcgetattr(m_descriptor, &m_restoredTermios) == -1) {
        dptr->setError(SerialPort::UnknownPortError);
        return false;
    }
    ::memcpy(&m_currentTermios, &m_restoredTermios, sizeof(struct termios));

    // Set other options.
    ::cfmakeraw(&m_currentTermios);
    m_currentTermios.c_cflag |= (CREAD | CLOCAL);
    m_currentTermios.c_cc[VTIME] = 0;

    // Apply new init settings.
    if (!updateTermios())
        return false;

    detectDefaultSettings();
    return true;
}

/*!
    Closes a serial port. Before closing, clears the exclusive access flag,
    removes the lock file, and restores the previous serial port settings
    if necessary.
*/
void UnixSerialPortEngine::close(const QString &location)
{
    // Restore saved port settings.
    if (dptr->options.restoreSettingsOnClose) {
        ::tcsetattr(m_descriptor, TCSANOW, &m_restoredTermios);
#if defined (Q_OS_LINUX)
        if (m_isCustomRateSupported)
            ::ioctl(m_descriptor, TIOCSSERIAL, &m_restoredSerialInfo);
#endif
    }

    // Try clean exclusive mode.
#if defined (TIOCNXCL)
    ::ioctl(m_descriptor, TIOCNXCL);
#endif

    ::close(m_descriptor);

    // Try unlock device by location.
    bool byCurrPid = false;
    if (TTYLocker::isLocked(location, &byCurrPid) && byCurrPid)
        TTYLocker::unlock(location);

    m_descriptor = -1;
    m_isCustomRateSupported = false;
}

/*!
    Returns a bitmap state of the RS-232 line signals. On error,
    the bitmap will be empty (equal zero).

    The POSIX API provides the state of all signals:
    LE, DTR, RTS, ST, SR, CTS, DCD, RING, and DSR. Of course, if the
    corresponding constants are defined on the particular platform.
*/
SerialPort::Lines UnixSerialPortEngine::lines() const
{
    int arg = 0;
    SerialPort::Lines ret = 0;

    if (::ioctl(m_descriptor, TIOCMGET, &arg) == -1) {
        // Print error?
        return ret;
    }

#if defined (TIOCLE)
    if (arg & TIOCLE) ret |= SerialPort::Le;
#endif
#if defined (TIOCDTR)
    if (arg & TIOCDTR) ret |= SerialPort::Dtr;
#endif
#if defined (TIOCRTS)
    if (arg & TIOCRTS) ret |= SerialPort::Rts;
#endif
#if defined (TIOCST)
    if (arg & TIOCST) ret |= SerialPort::St;
#endif
#if defined (TIOCSR)
    if (arg & TIOCSR) ret |= SerialPort::Sr;
#endif
#if defined (TIOCCTS)
    if (arg & TIOCCTS) ret |= SerialPort::Cts;
#endif

#if defined (TIOCCAR)
    if (arg & TIOCCAR) ret |= SerialPort::Dcd;
#elif defined (TIOCCD)
    if (arg & TIOCCD) ret |= SerialPort::Dcd;
#endif

#if defined (TIOCRNG)
    if (arg & TIOCRNG) ret |= SerialPort::Ri;
#elif defined (TIOCRI)
    if (arg & TIOCRI) ret |= SerialPort::Ri;
#endif

#if defined (TIOCDSR)
    if (arg & TIOCDSR) ret |= SerialPort::Dsr;
#endif

    return ret;
}

//
static bool trigger_out_line(int fd, int bit, bool set)
{
    int arg = 0;
    bool ret = (::ioctl(fd, TIOCMGET, &arg) != -1);

    if (ret) {
        int tmp = arg & bit;

        // If line already installed, then it no need change.
        if ((tmp && set) || (!(tmp || set)))
            return true;

        if (set)
            arg |= bit;
        else
            arg &= (~bit);

        ret = (::ioctl(fd, TIOCMSET, &arg) != -1);
    }
    return ret;
}

/*!
    Set DTR signal to state \a set.

    If successful, returns true; otherwise returns false.
*/
bool UnixSerialPortEngine::setDtr(bool set)
{
    bool ret = trigger_out_line(m_descriptor, TIOCM_DTR, set);
    if (!ret) {
        // FIXME: Here need call errno
        // and set error type.
    }
    return ret;
}

/*!
    Set RTS signal to state \a set.

    If successful, returns true; otherwise returns false.
*/
bool UnixSerialPortEngine::setRts(bool set)
{
    bool ret = trigger_out_line(m_descriptor, TIOCM_RTS, set);
    if (!ret) {
        // FIXME: Here need call errno
        // and set error type.
    }
    return ret;
}

/*!
    The shall block until all data output written to the serial
    port is transmitted.

    If successful, returns true; otherwise returns false.
*/
bool UnixSerialPortEngine::flush()
{
    bool ret = (::tcdrain(m_descriptor) != -1);
    if (!ret) {
        // FIXME: Here need call errno
        // and set error type.
    }
    return ret;
}

/*!
    Flushes both data received but not read and data written
    but not transmitted.

    If successful, returns true; otherwise returns false.
*/
bool UnixSerialPortEngine::reset()
{
    bool ret = (::tcflush(m_descriptor, TCIOFLUSH) != -1);
    if (!ret) {
        // FIXME: Here need call errno
        // and set error type.
    }
    return ret;
}

/*!
    Send a break for a specific \a duration.

    If successful, returns true; otherwise returns false.
*/
bool UnixSerialPortEngine::sendBreak(int duration)
{
    bool ret = (::tcsendbreak(m_descriptor, duration) != -1);
    if (!ret) {
        // FIXME: Here need call errno
        // and set error type.
    }
    return ret;
}

/*!
    Turn break on or off, that is, start or stop sending zero
    bits, depending on the parameter \a set.

    If successful, returns true; otherwise returns false.
*/
bool UnixSerialPortEngine::setBreak(bool set)
{
    bool ret = (::ioctl(m_descriptor, set ? TIOCSBRK : TIOCCBRK) != -1);
    if (!ret) {
        // FIXME: Here need call errno
        // and set error type.
    }
    return ret;
}

/*!
    If successful, returns the number of bytes in
    the input buffer; otherwise -1.
*/
qint64 UnixSerialPortEngine::bytesAvailable() const
{
    int nbytes = 0;
#if defined (TIOCINQ)
    if (::ioctl(m_descriptor, TIOCINQ, &nbytes) == -1)
        return -1;
#endif
    return nbytes;
}

/*!
    If successful, returns the number of bytes in
    the output buffer; otherwise -1.
*/
qint64 UnixSerialPortEngine::bytesToWrite() const
{
    int nbytes = 0;
#if defined (TIOCOUTQ)
    if (::ioctl(m_descriptor, TIOCOUTQ, &nbytes) == -1)
        return -1;
#endif
    return nbytes;
}

/*!
    Reads at most \a len bytes from the serial port into \a data, and returns
    the number of bytes read. If an error occurs, this function returns -1
    and sets an error code. This function returns immediately.

    Some platforms does not support the mark or space parity, so
    a software emulation of these modes is done while reading.

    Also, this method processed the policy of operating with the
    received symbol, in which the parity or frame error is detected.
*/
qint64 UnixSerialPortEngine::read(char *data, qint64 len)
{
    qint64 bytesRead = 0;
#if defined (CMSPAR)
    if ((dptr->options.parity == SerialPort::NoParity)
            || (dptr->options.policy != SerialPort::StopReceivingPolicy))
#else
    if ((dptr->options.parity != SerialPort::MarkParity)
            && (dptr->options.parity != SerialPort::SpaceParity))
#endif
        bytesRead = ::read(m_descriptor, data, len);
    else // Perform parity emulation.
        bytesRead = readPerChar(data, len);

    // FIXME: Here 'errno' codes for sockets.
    // You need to replace the codes for the serial port.
    if (bytesRead < 0) {
        bytesRead = -1;
        switch (errno) {
#if EWOULDBLOCK-0 && EWOULDBLOCK != EAGAIN
        case EWOULDBLOCK:
#endif
        case EAGAIN:
            // No data was available for reading.
            bytesRead = -2;
            break;
        case EBADF:
        case EINVAL:
        case EIO:
            break;
#if defined (Q_OS_SYMBIAN)
        case EPIPE:
#endif
        case ECONNRESET:
#if defined (Q_OS_VXWORKS)
        case ESHUTDOWN:
#endif
            bytesRead = 0;
            break;
        default:;
        }
    }
    return bytesRead;
}

/*!
    Writes at most \a len bytes of data from \a data to the serial port.
    If successful, returns the number of bytes that were actually written;
    otherwise returns -1 and sets an error code.

    Some platforms does not support the mark or space parity, so
    running software emulation of these modes while writing.
*/
qint64 UnixSerialPortEngine::write(const char *data, qint64 len)
{
    qint64 bytesWritten = 0;
#if defined (CMSPAR)
    bytesWritten = ::write(m_descriptor, data, len);
#else
    if ((dptr->options.parity != SerialPort::MarkParity) && (dptr->options.parity != SerialPort::SpaceParity))
        bytesWritten = ::write(m_descriptor, data, len);
    else // Perform parity emulation.
        bytesWritten = writePerChar(data, len);
#endif

    // FIXME: Here 'errno' codes for sockets.
    // You need to replace the codes for the serial port.
    if (bytesWritten < 0) {
        switch (errno) {
        case EPIPE:
        case ECONNRESET:
            bytesWritten = -1;
            break;
        case EAGAIN:
            bytesWritten = 0;
            break;
        case EMSGSIZE:
            break;
        default:;
        }
    }
    return bytesWritten;
}

/*!
    Implements a function blocking for waiting of events on the
    \a timeout in millisecond, those listed in fdread will be watched
    to see if characters become available for reading (more precisely,
    to see if a read will not block; in particular, a file m_descriptor
    is also ready on end-of-file), those in fdwrite will be watched
    to see if a write will not block.

    Event fdread controlled, if the flag \a checkRead is set on true,
    and fdwrite wehn flag \a checkWrite is set on true. The result
    of catch in each of the events, save to the corresponding
    variables \a selectForRead and \a selectForWrite.

    Returns true if the occurrence of any event before the timeout;
    otherwise returns false.
*/
bool UnixSerialPortEngine::select(int timeout,
                                  bool checkRead, bool checkWrite,
                                  bool *selectForRead, bool *selectForWrite)
{
    fd_set fdread;
    FD_ZERO(&fdread);
    if (checkRead)
        FD_SET(m_descriptor, &fdread);

    fd_set fdwrite;
    FD_ZERO(&fdwrite);
    if (checkWrite)
        FD_SET(m_descriptor, &fdwrite);

    struct timeval tv;
    tv.tv_sec = (timeout / 1000);
    tv.tv_usec = (timeout % 1000) * 1000;

    if (::select(m_descriptor + 1, &fdread, &fdwrite, 0, (timeout < 0) ? 0 : &tv) <= 0) {
        Q_ASSERT(selectForRead);
        *selectForRead = false;
        Q_ASSERT(selectForWrite);
        *selectForWrite = false;
        return false;
    }

    if (checkRead) {
        Q_ASSERT(selectForRead);
        *selectForRead = FD_ISSET(m_descriptor, &fdread);
    }

    if (checkWrite) {
        Q_ASSERT(selectForWrite);
        *selectForWrite = FD_ISSET(m_descriptor, &fdwrite);
    }
    return true;
}

#if defined (Q_OS_MAC)
static const QString defaultPathPrefix = QLatin1String("/dev/cu.");
static const QString notUsedPathPrefix = QLatin1String("/dev/tty.");
#else
static const QString defaultPathPrefix = QLatin1String("/dev/");
#endif

/*!
    Converts a platform specific \a port name to a system location
    and returns the value.
*/
QString UnixSerialPortEngine::toSystemLocation(const QString &port) const
{
    QString ret = port;

#if defined (Q_OS_MAC)
    ret.remove(notUsedPathPrefix);
#endif

    if (!ret.contains(defaultPathPrefix))
        ret.prepend(defaultPathPrefix);
    return ret;
}

/*!
    Converts a platform specific system \a location to a port name
    and returns the value.
*/
QString UnixSerialPortEngine::fromSystemLocation(const QString &location) const
{
    QString ret = location;

#if defined (Q_OS_MAC)
    ret.remove(notUsedPathPrefix);
#endif

    ret.remove(defaultPathPrefix);
    return ret;
}

/*!
    Sets the desired baud \a rate for the given direction \a dir,
    where \a rate is expressed by any positive integer type qint32.
    Also the method makes attempts to analyze the type of the desired
    standard or custom speed and sets the value.

    If successful, returns true; otherwise returns false and sets an
    error code.
*/
bool UnixSerialPortEngine::setRate(qint32 rate, SerialPort::Directions dir)
{
    bool ret = (rate > 0);

    // prepare section

    if (ret) {
        const qint32 unixRate = settingFromRate(rate);
        if (unixRate > 0) {
            // try prepate to set standard baud rate
#if defined (Q_OS_LINUX)
            // prepare to forcefully reset the custom mode
            if (m_isCustomRateSupported) {
                //m_currentSerialInfo.flags |= ASYNC_SPD_MASK;
                m_currentSerialInfo.flags &= ~(ASYNC_SPD_CUST /* | ASYNC_LOW_LATENCY*/);
                m_currentSerialInfo.custom_divisor = 0;
            }
#endif
            // prepare to set standard rate
            ret = !(((dir & SerialPort::Input) && (::cfsetispeed(&m_currentTermios, unixRate) < 0))
                    || ((dir & SerialPort::Output) && (::cfsetospeed(&m_currentTermios, unixRate) < 0)));
        } else {
            // try prepate to set custom baud rate
#if defined (Q_OS_LINUX)
            // prepare to forcefully set the custom mode
            if (m_isCustomRateSupported) {
                m_currentSerialInfo.flags &= ~ASYNC_SPD_MASK;
                m_currentSerialInfo.flags |= (ASYNC_SPD_CUST /* | ASYNC_LOW_LATENCY*/);
                m_currentSerialInfo.custom_divisor = m_currentSerialInfo.baud_base / rate;
                if (m_currentSerialInfo.custom_divisor == 0)
                    m_currentSerialInfo.custom_divisor = 1;
                // for custom mode needed prepare to set B38400 rate
                ret = (::cfsetspeed(&m_currentTermios, B38400) != -1);
            } else {
                ret = false;
            }
#elif defined (Q_OS_MAC)

#  if defined (MAC_OS_X_VERSION_10_4) && (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4)
            // Starting with Tiger, the IOSSIOSPEED ioctl can be used to set arbitrary baud rates
            // other than those specified by POSIX. The driver for the underlying serial hardware
            // ultimately determines which baud rates can be used. This ioctl sets both the input
            // and output speed.
            ret = (::ioctl(m_descriptor, IOSSIOSPEED, &rate) != -1);
#  else
            // others MacOSX version, can't prepare to set custom rate
            ret = false;
#  endif

#else
            // others *nix OS, can't prepare to set custom rate
            ret = false;
#endif
        }
    }

    // finally section

#if defined (Q_OS_LINUX)
    if (ret && m_isCustomRateSupported) {
        // finally, set or reset the custom mode
        ret = (::ioctl(m_descriptor, TIOCSSERIAL, &m_currentSerialInfo) != -1);
    }
#endif

    if (ret) {
        // finally, set rate
        ret = updateTermios();
    }

    if (!ret)
        dptr->setError(SerialPort::UnsupportedPortOperationError);
    return ret;
}

/*!
    Sets the desired number of data bits \a dataBits in a frame. POSIX
    supports all present number of data bits 5, 6, 7, and 8.

    If successful, returns true; otherwise returns false and sets an
    error code.
*/
bool UnixSerialPortEngine::setDataBits(SerialPort::DataBits dataBits)
{
    m_currentTermios.c_cflag &= (~CSIZE);
    switch (dataBits) {
    case SerialPort::Data5:
        m_currentTermios.c_cflag |= CS5;
        break;
    case SerialPort::Data6:
        m_currentTermios.c_cflag |= CS6;
        break;
    case SerialPort::Data7:
        m_currentTermios.c_cflag |= CS7;
        break;
    case SerialPort::Data8:
        m_currentTermios.c_cflag |= CS8;
        break;
    default:
        dptr->setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }
    return updateTermios();
}

/*!
    Sets the desired \a parity control mode. POSIX does not support
    the modes mark and space, so they are software emulated in the
    methods read() and write(). Some GNU/Linux variants have hardware
    support for these modes and don't need an emulation.

    If successful, returns true; otherwise returns false and sets an
    error code.
*/
bool UnixSerialPortEngine::setParity(SerialPort::Parity parity)
{
    m_currentTermios.c_iflag &= ~(PARMRK | INPCK);
    m_currentTermios.c_iflag |= IGNPAR;

    switch (parity) {

#if defined (CMSPAR)
    // Here Installation parity only for GNU/Linux where the macro CMSPAR.
    case SerialPort::SpaceParity:
        m_currentTermios.c_cflag &= (~PARODD);
        m_currentTermios.c_cflag |= (PARENB | CMSPAR);
        break;
    case SerialPort::MarkParity:
        m_currentTermios.c_cflag |= (PARENB | CMSPAR | PARODD);
        break;
#endif
    case SerialPort::NoParity:
        m_currentTermios.c_cflag &= (~PARENB);
        break;
    case SerialPort::EvenParity:
        m_currentTermios.c_cflag &= (~PARODD);
        m_currentTermios.c_cflag |= PARENB;
        break;
    case SerialPort::OddParity:
        m_currentTermios.c_cflag |= (PARENB | PARODD);
        break;
    default:
        m_currentTermios.c_cflag |= PARENB;
        m_currentTermios.c_iflag |= (PARMRK | INPCK);
        m_currentTermios.c_iflag &= ~IGNPAR;
        break;
    }

    return updateTermios();
}

/*!
    Sets the desired number of stop bits \a stopBits in a frame. POSIX
    only supports 1 or 2 stop bits.

    If successful, returns true; otherwise returns false and sets an
    error code.
*/
bool UnixSerialPortEngine::setStopBits(SerialPort::StopBits stopBits)
{
    switch (stopBits) {
    case SerialPort::OneStop:
        m_currentTermios.c_cflag &= (~CSTOPB);
        break;
    case SerialPort::TwoStop:
        m_currentTermios.c_cflag |= CSTOPB;
        break;
    default:
        dptr->setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }
    return updateTermios();
}

/*!
    Sets the desired \a flow control mode. POSIX supports all
    present flow control modes: no control, hardware (RTS/CTS),
    and software (XON/XOFF).

    If successful, returns true; otherwise returns false, and sets an
    error code.
*/
bool UnixSerialPortEngine::setFlowControl(SerialPort::FlowControl flow)
{
    switch (flow) {
    case SerialPort::NoFlowControl:
        m_currentTermios.c_cflag &= (~CRTSCTS);
        m_currentTermios.c_iflag &= (~(IXON | IXOFF | IXANY));
        break;
    case SerialPort::HardwareControl:
        m_currentTermios.c_cflag |= CRTSCTS;
        m_currentTermios.c_iflag &= (~(IXON | IXOFF | IXANY));
        break;
    case SerialPort::SoftwareControl:
        m_currentTermios.c_cflag &= (~CRTSCTS);
        m_currentTermios.c_iflag |= (IXON | IXOFF | IXANY);
        break;
    default:
        dptr->setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }
    return updateTermios();
}

/*!
    Sets the desired char error \a policy when frame or parity errors are
    detected.
*/
bool UnixSerialPortEngine::setDataErrorPolicy(SerialPort::DataErrorPolicy policy)
{
    tcflag_t parmrkMask = PARMRK;
#ifndef CMSPAR
    //in space/mark parity emulation also used PARMRK flag
    if (dptr->options.parity == SerialPort::SpaceParity || dptr->options.parity == SerialPort::MarkParity)
        parmrkMask = 0;
#endif //CMSPAR
    switch(policy) {
    case SerialPort::SkipPolicy:
        m_currentTermios.c_iflag &= ~parmrkMask;
        m_currentTermios.c_iflag |= IGNPAR | INPCK;
        break;
    case SerialPort::PassZeroPolicy:
        m_currentTermios.c_iflag &= ~(IGNPAR | parmrkMask);
        m_currentTermios.c_iflag |= INPCK;
        break;
    case SerialPort::IgnorePolicy:
        m_currentTermios.c_iflag &= ~INPCK;
        break;
    case SerialPort::StopReceivingPolicy:
        m_currentTermios.c_iflag &= ~IGNPAR;
        m_currentTermios.c_iflag |= (parmrkMask | INPCK);
        break;
    default:
        dptr->setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }
    return updateTermios();
}

/*!
    Returns the current read notification subsystem status.
*/
bool UnixSerialPortEngine::isReadNotificationEnabled() const
{
    return (m_readNotifier && m_readNotifier->isEnabled());
}

/*!
    Enables or disables the read notification subsystem, depending on
    the \a enable parameter. The enabled subsystem will asynchronously
    track the occurrence of the event fdread.
    Thereby, SerialPort can emit the signal readyRead() and automatically
    fill the internal receive buffer with new data that was received from
    the serial port in the event loop.
*/
void UnixSerialPortEngine::setReadNotificationEnabled(bool enable)
{
    if (m_readNotifier)
        m_readNotifier->setEnabled(enable);
    else if (enable) {
        m_readNotifier =
                new QSocketNotifier(m_descriptor, QSocketNotifier::Read, this);

        m_readNotifier->installEventFilter(this);
        m_readNotifier->setEnabled(true);
    }
}

/*!
    Returns the current write notification subsystem status.
*/
bool UnixSerialPortEngine::isWriteNotificationEnabled() const
{
    return (m_writeNotifier && m_writeNotifier->isEnabled());
}

/*!
    Enables or disables the write notification subsystem, depending on
    the \a enable parameter. The enabled subsystem will asynchronously
    track the occurrence of the event fdwrite.
    Thereby, SerialPort can automatically write data from the
    internal transfer buffer to the serial port in the event loop.
*/
void UnixSerialPortEngine::setWriteNotificationEnabled(bool enable)
{
    if (m_writeNotifier)
        m_writeNotifier->setEnabled(enable);
    else if (enable) {
        m_writeNotifier =
                new QSocketNotifier(m_descriptor, QSocketNotifier::Write, this);

        m_writeNotifier->installEventFilter(this);
        m_writeNotifier->setEnabled(true);
    }
}

/*!
    Returns the current error notification subsystem status.
*/
bool UnixSerialPortEngine::isErrorNotificationEnabled() const
{
    return (m_exceptionNotifier && m_exceptionNotifier->isEnabled());
}

/*!
    Enables or disables the error notification subsystem, depending on
    the \a enable parameter. The enabled subsystem will asynchronously
    track the occurrence of the event fderror.
*/
void UnixSerialPortEngine::setErrorNotificationEnabled(bool enable)
{
    if (m_exceptionNotifier)
        m_exceptionNotifier->setEnabled(enable);
    else if (enable) {
        m_exceptionNotifier =
                new QSocketNotifier(m_descriptor, QSocketNotifier::Exception, this);

        m_exceptionNotifier->installEventFilter(this);
        m_exceptionNotifier->setEnabled(true);
    }
}

/*!
    Not used in POSIX implementation, error handling is carried
    out in other ways.

    Always returns false.
*/
bool UnixSerialPortEngine::processIOErrors()
{
    // No need impl.
    return false;
}

/* Public static methods */

struct RatePair
{
   qint32 rate;    // The numerical value of baud rate.
   qint32 setting; // The OS-specific code of baud rate.
   bool operator<(const RatePair &other) const { return rate < other.rate; }
   bool operator==(const RatePair &other) const { return setting == other.setting; }
};

// This table contains correspondences standard pairs values of
// baud rates that are defined in file termios.h
static
const RatePair standardRatesTable[] =
{
    #if defined (B50)
    { 50, B50},
    #endif
    #if defined (B75)
    { 75, B75},
    #endif
    #if defined (B110)
    { 110, B110},
    #endif
    #if defined (B134)
    { 134, B134},
    #endif
    #if defined (B150)
    { 150, B150},
    #endif
    #if defined (B200)
    { 200, B200},
    #endif
    #if defined (B300)
    { 300, B300},
    #endif
    #if defined (B600)
    { 600, B600},
    #endif
    #if defined (B1200)
    { 1200, B1200},
    #endif
    #if defined (B1800)
    { 1800, B1800},
    #endif
    #if defined (B2400)
    { 2400, B2400},
    #endif
    #if defined (B4800)
    { 4800, B4800},
    #endif
    #if defined (B9600)
    { 9600, B9600},
    #endif
    #if defined (B19200)
    { 19200, B19200},
    #endif
    #if defined (B38400)
    { 38400, B38400},
    #endif
    #if defined (B57600)
    { 57600, B57600},
    #endif
    #if defined (B115200)
    { 115200, B115200},
    #endif
    #if defined (B230400)
    { 230400, B230400},
    #endif
    #if defined (B460800)
    { 460800, B460800},
    #endif
    #if defined (B500000)
    { 500000, B500000},
    #endif
    #if defined (B576000)
    { 576000, B576000},
    #endif
    #if defined (B921600)
    { 921600, B921600},
    #endif
    #if defined (B1000000)
    { 1000000, B1000000},
    #endif
    #if defined (B1152000)
    { 1152000, B1152000},
    #endif
    #if defined (B1500000)
    { 1500000, B1500000},
    #endif
    #if defined (B2000000)
    { 2000000, B2000000},
    #endif
    #if defined (B2500000)
    { 2500000, B2500000},
    #endif
    #if defined (B3000000)
    { 3000000, B3000000},
    #endif
    #if defined (B3500000)
    { 3500000, B3500000},
    #endif
    #if defined (B4000000)
    { 4000000, B4000000}
    #endif
};

static const RatePair *standardRatesTable_end =
        standardRatesTable + sizeof(standardRatesTable)/sizeof(*standardRatesTable);

/*!
    Converts the *nix-specific baud rate code \a setting to a numeric value.
    If the desired item is not found, returns 0.
*/
qint32 UnixSerialPortEngine::rateFromSetting(qint32 setting)
{
    const RatePair rp = {0, setting};
    const RatePair *ret = qFind(standardRatesTable, standardRatesTable_end, rp);
    return (ret != standardRatesTable_end) ? ret->rate : 0;
}

/*!
    Converts a numeric baud \a rate value to the *nix-specific code.
    If the desired item is not found, returns 0.
*/
qint32 UnixSerialPortEngine::settingFromRate(qint32 rate)
{
    const RatePair rp = {rate, 0};
    const RatePair *ret = qBinaryFind(standardRatesTable, standardRatesTable_end, rp);
    return (ret != standardRatesTable_end) ? ret->setting : 0;
}

/*!
    Returns a list of standard baud rate values, codes are defined in termios.h.
*/
QList<qint32> UnixSerialPortEngine::standardRates()
{
    QList<qint32> ret;
    for (const RatePair *it = standardRatesTable; it != standardRatesTable_end; ++it)
       ret.append(it->rate);
    return ret;
}

/* Protected methods */

/*!
    Attempts to determine the current serial port settings,
    when the port is opened. Used only in the method open().
*/
void UnixSerialPortEngine::detectDefaultSettings()
{
    // Detect rate.
    const speed_t inputUnixRate = ::cfgetispeed(&m_currentTermios);
    const speed_t outputUnixRate = ::cfgetispeed(&m_currentTermios);
    bool isCustomRateCurrentSet = false;

#if defined (Q_OS_LINUX)
    // try detect the ability to support custom rate
    m_isCustomRateSupported = (::ioctl(m_descriptor, TIOCGSERIAL, &m_currentSerialInfo) != -1)
            && (::ioctl(m_descriptor, TIOCSSERIAL, &m_currentSerialInfo) != -1);

    if (m_isCustomRateSupported) {

        ::memcpy(&m_restoredSerialInfo, &m_currentSerialInfo, sizeof(struct serial_struct));

        // assume that the baud rate is a custom
        isCustomRateCurrentSet = (inputUnixRate == B38400)
                && (outputUnixRate == B38400);

        if (isCustomRateCurrentSet) {
            if ((m_currentSerialInfo.flags & ASYNC_SPD_CUST)
                    && (m_currentSerialInfo.custom_divisor > 0)) {

                // yes, speed is really custom
                dptr->options.inputRate = m_currentSerialInfo.baud_base / m_currentSerialInfo.custom_divisor;
                dptr->options.outputRate = dptr->options.inputRate;
            } else {
                // no, we were wrong and the speed is a standard 38400 baud
                isCustomRateCurrentSet = false;
            }
        }
    }
#else
    // other *nix
#endif
    if (!m_isCustomRateSupported || !isCustomRateCurrentSet) {
        dptr->options.inputRate = rateFromSetting(inputUnixRate);
        dptr->options.outputRate = rateFromSetting(outputUnixRate);
    }

    // Detect databits.
    switch (m_currentTermios.c_cflag & CSIZE) {
    case CS5:
        dptr->options.dataBits = SerialPort::Data5;
        break;
    case CS6:
        dptr->options.dataBits = SerialPort::Data6;
        break;
    case CS7:
        dptr->options.dataBits = SerialPort::Data7;
        break;
    case CS8:
        dptr->options.dataBits = SerialPort::Data8;
        break;
    default:
        dptr->options.dataBits = SerialPort::UnknownDataBits;
    }

    // Detect parity.
#if defined (CMSPAR)
    if (m_currentTermios.c_cflag & CMSPAR) {
        dptr->options.parity = (m_currentTermios.c_cflag & PARODD) ?
                    SerialPort::MarkParity : SerialPort::SpaceParity;
    } else {
#endif
        if (m_currentTermios.c_cflag & PARENB) {
            dptr->options.parity = (m_currentTermios.c_cflag & PARODD) ?
                        SerialPort::OddParity : SerialPort::EvenParity;
        } else
            dptr->options.parity = SerialPort::NoParity;
#if defined (CMSPAR)
    }
#endif

    // Detect stopbits.
    dptr->options.stopBits = (m_currentTermios.c_cflag & CSTOPB) ?
                SerialPort::TwoStop : SerialPort::OneStop;

    // Detect flow control.
    if ((!(m_currentTermios.c_cflag & CRTSCTS)) && (!(m_currentTermios.c_iflag & (IXON | IXOFF | IXANY))))
        dptr->options.flow = SerialPort::NoFlowControl;
    else if ((!(m_currentTermios.c_cflag & CRTSCTS)) && (m_currentTermios.c_iflag & (IXON | IXOFF | IXANY)))
        dptr->options.flow = SerialPort::SoftwareControl;
    else if ((m_currentTermios.c_cflag & CRTSCTS) && (!(m_currentTermios.c_iflag & (IXON | IXOFF | IXANY))))
        dptr->options.flow = SerialPort::HardwareControl;
    else
        dptr->options.flow = SerialPort::UnknownFlowControl;

    //detect error policy
    if (m_currentTermios.c_iflag & INPCK) {
        if (m_currentTermios.c_iflag & IGNPAR)
            dptr->options.policy = SerialPort::SkipPolicy;
        else if (m_currentTermios.c_iflag & PARMRK)
            dptr->options.policy = SerialPort::StopReceivingPolicy;
        else
            dptr->options.policy = SerialPort::PassZeroPolicy;
    } else {
        dptr->options.policy = SerialPort::IgnorePolicy;
    }
}

/*!
    POSIX event loop for notification subsystem.
    Asynchronously in event loop continuous mode tracking the
    events from the serial port, as: fderror, fdread, and fdwrite.
    Calls the handler from the parent class SerialPortPrivate
    when a relevant event occurs.
*/
bool UnixSerialPortEngine::eventFilter(QObject *obj, QEvent *e)
{
    if (e->type() == QEvent::SockAct) {
        if (obj == m_readNotifier) {
            dptr->canReadNotification();
            return true;
        }
        if (obj == m_writeNotifier) {
            dptr->canWriteNotification();
            return true;
        }
        if (obj == m_exceptionNotifier) {
            dptr->canErrorNotification();
            return true;
        }
    }
    return QObject::eventFilter(obj, e);
}

/* Private methods */

/*!
    Updates the termios structure when changing any serial port parameter.

    If successful, returns true; otherwise returns false.
*/
bool UnixSerialPortEngine::updateTermios()
{
    if (::tcsetattr(m_descriptor, TCSANOW, &m_currentTermios) == -1) {
        dptr->setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }
    return true;
}

/* */
static inline bool evenParity(quint8 c)
{
    c ^= c >> 4;        //(c7 ^ c3)(c6 ^ c2)(c5 ^ c1)(c4 ^ c0)
    c ^= c >> 2;        //[(c7 ^ c3)(c5 ^ c1)][(c6 ^ c2)(c4 ^ c0)]
    c ^= c >> 1;
    return c & 1;       //(c7 ^ c3)(c5 ^ c1)(c6 ^ c2)(c4 ^ c0)
}

#if !defined (CMSPAR)

/*!
    For platforms that do not have the support of parities mark and space
    performed character by character emulation data transmitted, that
    one by one character is written to the port.
*/
qint64 UnixSerialPortEngine::writePerChar(const char *data, qint64 maxSize)
{
    qint64 ret = 0;
    quint8 const charMask = (0xFF >> (8 - dptr->options.dataBits));

    while (ret < maxSize) {

        bool par = evenParity(*data & charMask);
        // False if need EVEN, true if need ODD.
        par ^= (dptr->options.parity == SerialPort::MarkParity);
        if (par ^ bool(m_currentTermios.c_cflag & PARODD)) { // Need switch parity mode?
            m_currentTermios.c_cflag ^= PARODD;
            flush(); //force sending already buffered data, because updateTermios() cleares buffers
            //todo: add receiving buffered data!!!
            if (!updateTermios())
                break;
        }

        int r = ::write(m_descriptor, data, 1);
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

/*!
    Platforms which does not have the support for mark and space parity checking
    requires emulation using character by character data receiving.
*/
qint64 UnixSerialPortEngine::readPerChar(char *data, qint64 maxSize)
{
    qint64 ret = 0;
    quint8 const charMask = (0xFF >> (8 - dptr->options.dataBits));

    // 0 - prefix not started,
    // 1 - received 0xFF,
    // 2 - received 0xFF and 0x00
    int prefix = 0;
    while (ret < maxSize) {

        qint64 r = ::read(m_descriptor, data, 1);
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
        par ^= bool(m_currentTermios.c_cflag & PARODD); //par contains parity bit value for current mode
        if (par ^ bool(dptr->options.parity == SerialPort::SpaceParity)) { //if parity error
            switch (dptr->options.policy) {
            case SerialPort::SkipPolicy:
                continue;       //ignore received character
            case SerialPort::StopReceivingPolicy:
                if (dptr->options.parity != SerialPort::NoParity)
                    dptr->portError = SerialPort::ParityError;
                else
                    dptr->portError = (*data == '\0') ? SerialPort::BreakConditionError : SerialPort::FramingError;
                return ++ret;   //abort receiving
                break;
            case SerialPort::UnknownPolicy:
                qWarning() << "Unknown error policy is used! Falling back to PassZeroPolicy";
            case SerialPort::PassZeroPolicy:
                *data = '\0';   //replace received character by zero
                break;
            case SerialPort::IgnorePolicy:
                break;          //ignore error and pass received character
            }
        }
        ++data;
        ++ret;
    }
    return ret;
}

// From <serialportengine_p.h>
SerialPortEngine *SerialPortEngine::create(SerialPortPrivate *d)
{
    return new UnixSerialPortEngine(d);
}

#include "moc_serialportengine_unix_p.cpp"

QT_END_NAMESPACE_SERIALPORT
