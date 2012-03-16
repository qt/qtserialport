/*
    License...
*/

/*!
    \class UnixSerialPortEngine
    \internal

    \brief The UnixSerialPortEngine class provides *nix OS
    platform-specific low level access to a serial port.

    \reentrant
    \ingroup serialport-main
    \inmodule QtSerialPort

    Currently the class supports all POSIX-compatible OS (GNU/Linux, *BSD,
    Mac OSX and etc).

    UnixSerialPortEngine (as well as other platform-dependent engines)
    is a class with multiple inheritance, which on the one hand,
    derives from a general abstract class interface SerialPortEngine,
    on the other hand of a class inherited from QObject.

    From the abstract class SerialPortEngine, it inherits all virtual
    interface methods that are common to all serial ports on any platform.
    These methods, the class UnixSerialPortEngine implements use
    POSIX ABI.

    From QObject-like class, it inherits a specific system Qt features.
    For example, to track of events from a serial port uses the virtual
    QObject method eventFilter(), who the make analysis of the events
    from the type classes QSocketNotifier.

    That is, as seen from the above, the functional UnixSerialPortEngine
    completely covers all the necessary tasks.
*/

#include "serialportengine_unix_p.h"
#include "ttylocker_unix_p.h"

#include <errno.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#if defined (Q_OS_LINUX)
#  include <linux/serial.h>
#elif defined (Q_OS_MAC)
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
    Constructs a UnixSerialPortEngine with \a parent and
    initializes all the internal variables of the initial values.

    A pointer \a parent to the object class SerialPortPrivate
    is required for the recursive call some of its methods.
*/
UnixSerialPortEngine::UnixSerialPortEngine(SerialPortPrivate *parent)
    : m_descriptor(-1)
    , m_readNotifier(0)
    , m_writeNotifier(0)
    , m_exceptionNotifier(0)
{
    Q_ASSERT(parent);
    m_parent = parent;
    int size = sizeof(struct termios);
    ::memset(&m_currTermios, 0, size);
    ::memset(&m_oldTermios, 0, size);
}

/*!
    Stops the tracking events of the serial port and
    destructs a UnixSerialPortEngine.
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
    Tries to open the descriptor desired serial port by \a location
    in the given open \a mode.

    Before the opening of the serial port, checking for on exists the
    appropriate lock the file and the information therein. If the
    lock file is present, and the information contained in it is
    relevant - it is concluded that the current serial port is
    already occupied.

    In the process of discovery, always set a serial port in
    non-blocking mode (when the read operation returns immediately)
    and tries to determine its current configuration and install them.

    Since the port in the POSIX OS by default opens in shared mode,
    then this method forcibly puts a port in exclusive mode access.
    This is done simultaneously in two ways:
    - set to the pre-open descriptor a flag TIOCEXCL
    - creates a lock file, which writes the pid of the process, that
    opened the port and other information

    Need to use two methods due to the fact that on some platforms can
    not be defined constant TIOCEXCL, in this case, try to use the
    lock file. Creation and analysis of lock file, by using a special
    helper class TTYLocker.

    If successful, returns true; otherwise false, with the setup a
    error code.
*/
bool UnixSerialPortEngine::open(const QString &location, QIODevice::OpenMode mode)
{
    // First, here need check locked device or not.
    bool byCurrPid = false;
    if (TTYLocker::isLocked(location, &byCurrPid)) {
        m_parent->setError(SerialPort::PermissionDeniedError);
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
            m_parent->setError(SerialPort::NoSuchDeviceError);
            break;
        case EACCES:
            m_parent->setError(SerialPort::PermissionDeniedError);
            break;
        default:
            m_parent->setError(SerialPort::UnknownPortError);
        }
        return false;
    }

    // Try lock device by location and check it state is locked.
    TTYLocker::lock(location);
    if (!TTYLocker::isLocked(location, &byCurrPid)) {
        m_parent->setError(SerialPort::PermissionDeniedError);
        return false;
    }

    // Try set exclusive mode.
#if defined (TIOCEXCL)
    ::ioctl(m_descriptor, TIOCEXCL);
#endif

    // Save current port settings.
    if (::tcgetattr(m_descriptor, &m_oldTermios) == -1) {
        m_parent->setError(SerialPort::UnknownPortError);
        return false;
    }
    ::memcpy(&m_currTermios, &m_oldTermios, sizeof(struct termios));

    // Set other options.
    ::cfmakeraw(&m_currTermios);
    m_currTermios.c_cflag |= (CREAD | CLOCAL);
    m_currTermios.c_cc[VTIME] = 0;

    // Apply new init settings.
    if (!updateTermios())
        return false;

    detectDefaultSettings();
    return true;
}

/*!
    Closes a serial port descriptor. Before closing - clears exclusive
    access flag and removes the lock file, restore previous serial port
    settings if necessary.
*/
void UnixSerialPortEngine::close(const QString &location)
{
    // Restore saved port settings.
    if (m_parent->m_restoreSettingsOnClose)
        ::tcsetattr(m_descriptor, TCSANOW, &m_oldTermios);

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
}

/*!
    Returns a bitmap state of RS-232 line signals. On error,
    bitmap will be empty (equal zero).

    POSIX ABI allows you to receive all the state of signals:
    LE, DTR, RTS, ST, SR, CTS, DCD, RING, DSR. Of course, if the
    corresponding constants are defined in a particular platform.
*/
SerialPort::Lines UnixSerialPortEngine::lines() const
{
    int arg = 0;
    SerialPort::Lines ret = 0;

    if (::ioctl(m_descriptor, TIOCMGET, &arg) == -1) {
        // Print error?
        return ret;
    }

#if defined (TIOCM_LE)
    if (arg & TIOCM_LE) ret |= SerialPort::Le;
#endif
#if defined (TIOCM_DTR)
    if (arg & TIOCM_DTR) ret |= SerialPort::Dtr;
#endif
#if defined (TIOCM_RTS)
    if (arg & TIOCM_RTS) ret |= SerialPort::Rts;
#endif
#if defined (TIOCM_ST)
    if (arg & TIOCM_ST) ret |= SerialPort::St;
#endif
#if defined (TIOCM_SR)
    if (arg & TIOCM_SR) ret |= SerialPort::Sr;
#endif
#if defined (TIOCM_CTS)
    if (arg & TIOCM_CTS) ret |= SerialPort::Cts;
#endif

#if defined (TIOCM_CAR)
    if (arg & TIOCM_CAR) ret |= SerialPort::Dcd;
#elif defined (TIOCM_CD)
    if (arg & TIOCM_CD) ret |= SerialPort::Dcd;
#endif

#if defined (TIOCM_RNG)
    if (arg & TIOCM_RNG) ret |= SerialPort::Ri;
#elif defined (TIOCM_RI)
    if (arg & TIOCM_RI) ret |= SerialPort::Ri;
#endif

#if defined (TIOCM_DSR)
    if (arg & TIOCM_DSR) ret |= SerialPort::Dsr;
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

    If successful, returns true; otherwise false.
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

    If successful, returns true; otherwise false.
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

    If successful, returns true; otherwise false.
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

    If successful, returns true; otherwise false.
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

    If successful, returns true; otherwise false.
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

    If successful, returns true; otherwise false.
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
    If successful, returns the number of bytes that are
    immediately available for reading; otherwise -1.
*/
qint64 UnixSerialPortEngine::bytesAvailable() const
{
    int cmd = 0;
#if defined (FIONREAD)
    cmd = FIONREAD;
#else
    cmd = TIOCINQ;
#endif
    qint64 nbytes = 0;
    if (::ioctl(m_descriptor, cmd, &nbytes) == -1)
        return -1;
    return nbytes;
}

/*!
    Not supported on POSIX-compatible platform,
    always returns 0.
*/
qint64 UnixSerialPortEngine::bytesToWrite() const
{
    // FIXME: FIONWRITE (or analogy) is exists?
    return 0;
}

/*!
    If successful, returns to the external buffer \a data the
    real number of bytes read, which can be less than the
    requested \a len; otherwise returned -1 with set error code.
    In any case, reading function returns immediately.

    Some platforms do not support the mark or space parity, so
    running software emulation of these modes in the process of
    reading.

    Also, this method processed the policy of operating with the
    received symbol, in which the parity or frame error is detected.
*/
qint64 UnixSerialPortEngine::read(char *data, qint64 len)
{
    qint64 bytesRead = 0;
#if defined (CMSPAR)
    if ((m_parent->m_parity == SerialPort::NoParity) || (m_parent->m_policy != SerialPort::StopReceivingPolicy))
#else
    if ((m_parent->m_parity != SerialPort::MarkParity) && (m_parent->m_parity != SerialPort::SpaceParity))
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
        // FIXME: Here need call errno
        // and set error type?
        if (bytesRead == -1)
            m_parent->setError(SerialPort::IoError);
    }
    return bytesRead;
}

/*!
    Write \a data to serial port. If successful, returns the
    real number of bytes write, which can be less than the
    requested \a len; otherwise returned -1 with set error code.

    Some platforms do not support the mark or space parity, so
    running software emulation of these modes in the process of
    writting.
*/
qint64 UnixSerialPortEngine::write(const char *data, qint64 len)
{
    qint64 bytesWritten = 0;
#if defined (CMSPAR)
    bytesWritten = ::write(m_descriptor, data, len);
#else
    if ((m_parent->m_parity != SerialPort::MarkParity) && (m_parent->m_parity != SerialPort::SpaceParity))
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
        // FIXME: Here need call errno
        // and set error type?
        if (bytesWritten == -1)
            m_parent->setError(SerialPort::IoError);
    }
    return bytesWritten;
}

/*!
    Implements a function blocking for waiting of events on the
    \a timeout in millisecond, those listed in fdread will be watched
    to see if characters become available for reading (more precisely,
    to see if a read will not block; in particular, a file descriptor
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
    Converts a platform specific \a port name to system location
    and return result.
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
    Converts a platform specific system \a location to port name
    and return result.
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

static QHash<qint32, qint32> generateStandardRatesMachTable()
{
   QHash<qint32, qint32> h;
#if defined (B0)
    h[0] = B0;
#endif
#if defined (B50)
    h[50] = B50;
#endif
#if defined (B75)
    h[75] = B75;
#endif
#if defined (B110)
    h[110] = B110;
#endif
#if defined (B134)
    h[134] = B134;
#endif
#if defined (B150)
    h[150] = B150;
#endif
#if defined (B200)
    h[2000] = B200;
#endif
#if defined (B300)
    h[300] = B300;
#endif
#if defined (B600)
    h[600] = B600;
#endif
#if defined (B1200)
    h[1200] = B1200;
#endif
#if defined (B1800)
    h[1800] = B1800;
#endif
#if defined (B2400)
    h[2400] = B2400;
#endif
#if defined (B4800)
    h[4800] = B4800;
#endif
#if defined (B9600)
    h[9600] = B9600;
#endif
#if defined (B19200)
    h[19200] = B19200;
#endif
#if defined (B38400)
    h[38400] = B38400;
#endif
#if defined (B57600)
    h[57600] = B57600;
#endif
#if defined (B115200)
    h[115200] = B115200;
#endif
#if defined (B230400)
    h[230400] = B230400;
#endif
#if defined (B460800)
    h[460800] = B460800;
#endif
#if defined (B500000)
    h[500000] = B500000;
#endif
#if defined (B576000)
    h[576000] = B576000;
#endif
#if defined (B921600)
    h[921600] = B921600;
#endif
#if defined (B1000000)
    h[1000000] = B1000000;
#endif
#if defined (B1152000)
    h[1152000] = B1152000;
#endif
#if defined (B1500000)
    h[1500000] = B1500000;
#endif
#if defined (B2000000)
    h[2000000] = B2000000;
#endif
#if defined (B2500000)
    h[2500000] = B2500000;
#endif
#if defined (B3000000)
    h[3000000] = B3000000;
#endif
#if defined (B3500000)
    h[3500000] = B3500000;
#endif
#if defined (B4000000)
    h[4000000] = B4000000;
#endif
    return h;
}

// Standard baud rates match table, where:
// - key - is the numerical value of baud rate (eg. 9600, 115200 ...)
// - value - is the value of POSIX baud rate macros (eg. B9600, B115200 ...)
inline QHash<qint32, qint32>& standardRatesMachTable()
{
    static QHash<qint32, qint32> h = generateStandardRatesMachTable();
    return h;
}

/*!
    Set desired \a rate by given direction \a dir,
    where \a rate is expressed by any positive integer type qint32.
    Also the method make attempts to analyze the type of the desired
    standard or custom speed and trying to set it.

    If successful, returns true; otherwise false, with the setup a
    error code.
*/
bool UnixSerialPortEngine::setRate(qint32 rate, SerialPort::Directions dir)
{
    bool ret = (rate > 0);

    // prepare section

#if defined (Q_OS_LINUX)
    struct serial_struct ser_info;
    if (ret)
        ret = (::ioctl(m_descriptor, TIOCGSERIAL, &ser_info) != -1);
#endif

    if (ret) {
        const qint32 unixRate = standardRatesMachTable().value(rate);
        if (unixRate > 0) {
            // try prepate to set standard baud rate

#if defined (Q_OS_LINUX)
            // prepare to forcefully reset the custom mode
            ser_info.flags |= ASYNC_SPD_MASK;
            ser_info.flags &= ~(ASYNC_SPD_CUST /* | ASYNC_LOW_LATENCY*/);
            ser_info.custom_divisor = 0;
#endif
            // prepare to set standard rate
            ret = !(((dir & SerialPort::Input) && (::cfsetispeed(&m_currTermios, unixRate) < 0))
                    || ((dir & SerialPort::Output) && (::cfsetospeed(&m_currTermios, unixRate) < 0)));
        } else {
            // try prepate to set custom baud rate

#if defined (Q_OS_LINUX)
            // prepare to forcefully set the custom mode
            ser_info.flags &= ~ASYNC_SPD_MASK;
            ser_info.flags |= (ASYNC_SPD_CUST /* | ASYNC_LOW_LATENCY*/);
            ser_info.custom_divisor = ser_info.baud_base / rate;
            if (ser_info.custom_divisor == 0)
                ser_info.custom_divisor = 1;

            // for custom mode needed prepare to set B38400 rate
            ret = (::cfsetspeed(&m_currTermios, B38400) != -1);
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
    if (ret) {
        // finally, set or reset the custom mode
        ret = (::ioctl(m_descriptor, TIOCSSERIAL, &ser_info) != -1);
    }
#endif

    if (ret) {
        // finally, set rate
        ret = updateTermios();
    }

    if (!ret)
        m_parent->setError(SerialPort::UnsupportedPortOperationError);
    return ret;
}

/*!
    Set desired number of data bits \a dataBits in byte. POSIX
    native supported all present number of data bits 5, 6, 7, 8.

    If successful, returns true; otherwise false, with the setup a
    error code.
*/
bool UnixSerialPortEngine::setDataBits(SerialPort::DataBits dataBits)
{
    m_currTermios.c_cflag &= (~CSIZE);
    switch (dataBits) {
    case SerialPort::Data5:
        m_currTermios.c_cflag |= CS5;
        break;
    case SerialPort::Data6:
        m_currTermios.c_cflag |= CS6;
        break;
    case SerialPort::Data7:
        m_currTermios.c_cflag |= CS7;
        break;
    case SerialPort::Data8:
        m_currTermios.c_cflag |= CS8;
        break;
    default:
        m_parent->setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }
    return updateTermios();
}

/*!
    Set desired \a parity control mode. POSIX native not supported
    modes mark and space, so is their software emulation in the
    methods read() and write(). But, in particular, some GNU/Linux
    has hardware support for these modes, therefore, no need to
    emulate.

    If successful, returns true; otherwise false, with the setup a
    error code.
*/
bool UnixSerialPortEngine::setParity(SerialPort::Parity parity)
{
    m_currTermios.c_iflag &= ~(PARMRK | INPCK);
    m_currTermios.c_iflag |= IGNPAR;

    switch (parity) {

#if defined (CMSPAR)
    // Here Installation parity only for GNU/Linux where the macro CMSPAR.
    case SerialPort::SpaceParity:
        m_currTermios.c_cflag &= (~PARODD);
        m_currTermios.c_cflag |= (PARENB | CMSPAR);
        break;
    case SerialPort::MarkParity:
        m_currTermios.c_cflag |= (PARENB | CMSPAR | PARODD);
        break;
#endif
    case SerialPort::NoParity:
        m_currTermios.c_cflag &= (~PARENB);
        break;
    case SerialPort::EvenParity:
        m_currTermios.c_cflag &= (~PARODD);
        m_currTermios.c_cflag |= PARENB;
        break;
    case SerialPort::OddParity:
        m_currTermios.c_cflag |= (PARENB | PARODD);
        break;
    default:
        m_currTermios.c_cflag |= PARENB;
        m_currTermios.c_iflag |= (PARMRK | INPCK);
        m_currTermios.c_iflag &= ~IGNPAR;
        break;
    }

    return updateTermios();
}

/*!
    Set desired number of stop bits \a stopBits in frame. POSIX
    native supported only 1, 2 number of stop bits.

    If successful, returns true; otherwise false, with the setup a
    error code.
*/
bool UnixSerialPortEngine::setStopBits(SerialPort::StopBits stopBits)
{
    switch (stopBits) {
    case SerialPort::OneStop:
        m_currTermios.c_cflag &= (~CSTOPB);
        break;
    case SerialPort::TwoStop:
        m_currTermios.c_cflag |= CSTOPB;
        break;
    default:
        m_parent->setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }
    return updateTermios();
}

/*!
    Set desired \a flow control mode. POSIX  native supported all
    present flow control modes no control, hardware (RTS/CTS),
    software (XON/XOFF).

    If successful, returns true; otherwise false, with the setup a
    error code.
*/
bool UnixSerialPortEngine::setFlowControl(SerialPort::FlowControl flow)
{
    switch (flow) {
    case SerialPort::NoFlowControl:
        m_currTermios.c_cflag &= (~CRTSCTS);
        m_currTermios.c_iflag &= (~(IXON | IXOFF | IXANY));
        break;
    case SerialPort::HardwareControl:
        m_currTermios.c_cflag |= CRTSCTS;
        m_currTermios.c_iflag &= (~(IXON | IXOFF | IXANY));
        break;
    case SerialPort::SoftwareControl:
        m_currTermios.c_cflag &= (~CRTSCTS);
        m_currTermios.c_iflag |= (IXON | IXOFF | IXANY);
        break;
    default:
        m_parent->setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }
    return updateTermios();
}

/*!
    Set desired char error \a policy when errors are detected
    frame or parity.
*/
bool UnixSerialPortEngine::setDataErrorPolicy(SerialPort::DataErrorPolicy policy)
{
    tcflag_t parmrkMask = PARMRK;
#ifndef CMSPAR
    //in space/mark parity emulation also used PARMRK flag
    if (m_parent->m_parity == SerialPort::SpaceParity || m_parent->m_parity == SerialPort::MarkParity)
        parmrkMask = 0;
#endif //CMSPAR
    switch(policy) {
    case SerialPort::SkipPolicy:
        m_currTermios.c_iflag &= ~parmrkMask;
        m_currTermios.c_iflag |= IGNPAR | INPCK;
        break;
    case SerialPort::PassZeroPolicy:
        m_currTermios.c_iflag &= ~(IGNPAR | parmrkMask);
        m_currTermios.c_iflag |= INPCK;
        break;
    case SerialPort::IgnorePolicy:
        m_currTermios.c_iflag &= ~INPCK;
        break;
    case SerialPort::StopReceivingPolicy:
        m_currTermios.c_iflag &= ~IGNPAR;
        m_currTermios.c_iflag |= (parmrkMask | INPCK);
        break;
    default:
        m_parent->setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }
    return updateTermios();
}

/*!
    Returns the current status of the read notification subsystem.
*/
bool UnixSerialPortEngine::isReadNotificationEnabled() const
{
    return (m_readNotifier && m_readNotifier->isEnabled());
}

/*!
    Enables or disables read notification subsystem, depending on
    the \a enable parameter. If the subsystem is enabled, it will
    asynchronously track the occurrence of an event fdread.
    Thanks to that, SerialPort can emit a signal readyRead() and
    fill up the internal receive buffer with new data, that
    automatically received from a serial port in the event loop.
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
    Returns the current status of the write notification subsystem.
*/
bool UnixSerialPortEngine::isWriteNotificationEnabled() const
{
    return (m_writeNotifier && m_writeNotifier->isEnabled());
}

/*!
    Enables or disables write notification subsystem, depending on
    the \a enable parameter. If the subsystem is enabled, it will
    asynchronously track the occurrence of an event fdwrite.
    Thanks to that, SerialPort can write data from internal transfer
    buffer, to serial port automatically in the event loop.
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
    Not used in POSIX implementation, error handling is carried
    out in other ways.

    Always returned false.
*/
bool UnixSerialPortEngine::processIOErrors()
{
    // No need impl.
    return false;
}

/* Protected methods */

/*!
    Attempts to determine the current settings of the serial port,
    wehn it opened. Used only in the method open().
*/
void UnixSerialPortEngine::detectDefaultSettings()
{
    // Detect rate.
    bool isCustomRate = false;
#if defined (Q_OS_LINUX)
    // first assume that the baud rate of custom
    isCustomRate = ((::cfgetispeed(&m_currTermios) == B38400)
                    && (::cfgetospeed(&m_currTermios) == B38400));

    if (isCustomRate) {
        struct serial_struct ser_info;
        if (::ioctl(m_descriptor, TIOCGSERIAL, &ser_info) != -1) {
            if ((ser_info.flags & ASYNC_SPD_CUST)
                    && (ser_info.custom_divisor > 0)) {

                // yes, speed is really custom
                m_parent->m_inRate = ser_info.baud_base / ser_info.custom_divisor;
            } else {
                // no, we were wrong and the speed of a standard 38400 baud
                m_parent->m_inRate = 38400;
            }
        } else {
            // error get ioctl()
            m_parent->m_inRate = SerialPort::UnknownRate;
        }
        m_parent->m_outRate = m_parent->m_inRate;
    }
#else
    // other *nix
#endif
    if (!isCustomRate) {
        m_parent->m_inRate = standardRatesMachTable().key(::cfgetispeed(&m_currTermios));
        m_parent->m_outRate = standardRatesMachTable().key(::cfgetospeed(&m_currTermios));
    }

    // Detect databits.
    switch (m_currTermios.c_cflag & CSIZE) {
    case CS5:
        m_parent->m_dataBits = SerialPort::Data5;
        break;
    case CS6:
        m_parent->m_dataBits = SerialPort::Data6;
        break;
    case CS7:
        m_parent->m_dataBits = SerialPort::Data7;
        break;
    case CS8:
        m_parent->m_dataBits = SerialPort::Data8;
        break;
    default:
        m_parent->m_dataBits = SerialPort::UnknownDataBits;
    }

    // Detect parity.
#if defined (CMSPAR)
    if (m_currTermios.c_cflag & CMSPAR) {
        m_parent->m_parity = (m_currTermios.c_cflag & PARODD) ?
                    SerialPort::MarkParity : SerialPort::SpaceParity;
    } else {
#endif
        if (m_currTermios.c_cflag & PARENB) {
            m_parent->m_parity = (m_currTermios.c_cflag & PARODD) ?
                        SerialPort::OddParity : SerialPort::EvenParity;
        } else
            m_parent->m_parity = SerialPort::NoParity;
#if defined (CMSPAR)
    }
#endif

    // Detect stopbits.
    m_parent->m_stopBits = (m_currTermios.c_cflag & CSTOPB) ?
                SerialPort::TwoStop : SerialPort::OneStop;

    // Detect flow control.
    if ((!(m_currTermios.c_cflag & CRTSCTS)) && (!(m_currTermios.c_iflag & (IXON | IXOFF | IXANY))))
        m_parent->m_flow = SerialPort::NoFlowControl;
    else if ((!(m_currTermios.c_cflag & CRTSCTS)) && (m_currTermios.c_iflag & (IXON | IXOFF | IXANY)))
        m_parent->m_flow = SerialPort::SoftwareControl;
    else if ((m_currTermios.c_cflag & CRTSCTS) && (!(m_currTermios.c_iflag & (IXON | IXOFF | IXANY))))
        m_parent->m_flow = SerialPort::HardwareControl;
    else
        m_parent->m_flow = SerialPort::UnknownFlowControl;

    //detect error policy
    if (m_currTermios.c_iflag & INPCK) {
        if (m_currTermios.c_iflag & IGNPAR)
            m_parent->m_policy = SerialPort::SkipPolicy;
        else if (m_currTermios.c_iflag & PARMRK)
            m_parent->m_policy = SerialPort::StopReceivingPolicy;
        else
            m_parent->m_policy = SerialPort::PassZeroPolicy;
    } else {
        m_parent->m_policy = SerialPort::IgnorePolicy;
    }
}

/*!
    POSIX event loop for notification subsystem.
    Asynchronously in event loop continuous mode tracking the
    events from the serial port, as: fderror, fdread, fdwrite.
    When is occur a relevant event, calls him handler from
    a parent class SerialPortPrivate.
*/
bool UnixSerialPortEngine::eventFilter(QObject *obj, QEvent *e)
{
    if (e->type() == QEvent::SockAct) {
        if (obj == m_readNotifier) {
            m_parent->canReadNotification();
            return true;
        }
        if (obj == m_writeNotifier) {
            m_parent->canWriteNotification();
            return true;
        }
        if (obj == m_exceptionNotifier) {
            m_parent->canErrorNotification();
            return true;
        }
    }
    return QObject::eventFilter(obj, e);
}

/* Private methods */

/*!
    Updates the termios structure wehn changing of any the
    parameters a serial port.

    If successful, returns true; otherwise false.
*/
bool UnixSerialPortEngine::updateTermios()
{
    if (::tcsetattr(m_descriptor, TCSANOW, &m_currTermios) == -1) {
        m_parent->setError(SerialPort::UnsupportedPortOperationError);
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
    quint8 const charMask = (0xFF >> (8 - m_parent->m_dataBits));

    while (ret < maxSize) {

        bool par = evenParity(*data & charMask);
        // False if need EVEN, true if need ODD.
        par ^= (m_parent->m_parity == SerialPort::MarkParity);
        if (par ^ bool(m_currTermios.c_cflag & PARODD)) { // Need switch parity mode?
            m_currTermios.c_cflag ^= PARODD;
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
    quint8 const charMask = (0xFF >> (8 - m_parent->m_dataBits));

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
        par ^= bool(m_currTermios.c_cflag & PARODD); //par contains parity bit value for current mode
        if (par ^ bool(m_parent->m_parity == SerialPort::SpaceParity)) { //if parity error
            switch(m_parent->m_policy) {
            case SerialPort::SkipPolicy:
                continue;       //ignore received character
            case SerialPort::StopReceivingPolicy:
                if (m_parent->m_parity != SerialPort::NoParity)
                    m_parent->m_portError = SerialPort::ParityError;
                else
                    m_parent->m_portError = (*data == '\0') ? SerialPort::BreakConditionError : SerialPort::FramingError;
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
SerialPortEngine *SerialPortEngine::create(SerialPortPrivate *parent)
{
    return new UnixSerialPortEngine(parent);
}

#include "moc_serialportengine_unix_p.cpp"

QT_END_NAMESPACE_SERIALPORT
