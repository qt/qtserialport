/*
    License...
*/

#include "serialport_p.h"

#include <errno.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#if defined (Q_OS_LINUX)
#include <linux/serial.h>
#endif

#include <QtCore/QRegExp>


/* Public methods */


SerialPortPrivate::SerialPortPrivate()
    : SerialPortPortPrivate()
    , m_descriptor(-1)
{
    int size = sizeof(struct termios);
    ::memset(&m_currTermios, 0, size);
    ::memset(&m_oldTermios, 0, size);

    m_notifier.setRef(this);
}

bool SerialPortPrivate::open(QIODevice::OpenMode mode)
{
    // First, here need check locked device or not.
    bool locked = false;

    if (locked) {
        setError(SerialPort::PermissionDeniedError);
        return false;
    }

    int flags = (O_NOCTTY | O_NDELAY);

    if (mode & QIODevice::ReadOnly)
        flags |= O_RDONLY;
    if (mode & QIODevice::WriteOnly)
        flags |= O_WRONLY;

    // Try opened serial device.
    m_descriptor = ::open(m_systemLocation.toLocal8Bit().constData(), flags);

    // FIXME:
    /*
    if (m_descriptor == -1) {
        switch (errno) {
        case "file not found" :
            setError(SerialPort::NoSuchDeviceError); break;
        case "access denied":
            setError(SerialPort::PermissionDeniedError); break;
        default:
            setError(SerialPort::UnknownPortError);
        }
        return false;
    }
    */

    // Try lock device by location and check it state is locked.
    //...

    // Try set exclusive mode.
#if defined (TIOCEXCL)
    ::ioctl(m_descriptor, TIOCEXCL);
#endif

    if (saveOldSettings()) {

        prepareOtherOptions();
        prepareTimeouts(0);

        if (updateTermious()) {
            // Disable autocalculate total read interval.
            // isAutoCalcReadTimeoutConstant = false;

            detectDefaultCurrentSettings();
            return true;
        }
    }
}

void SerialPortPrivate::close()
{
    restoreOldsettings();

    // Try clean exclusive mode.
#if defined (TIOCNXCL)
    ::ioctl(m_descriptor, TIOCNXCL);
#endif

    ::close(m_descriptor);

    // Try unlock device by location.
    //...

    m_descriptor = -1;
}

SerialPort::Lines SerialPortPrivate::lines() const
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

bool SerialPortPrivate::setDtr(bool set)
{
    bool ret = trigger_out_line(m_descriptor, TIOCM_DTR, set);
    if (!ret) {
        // FIXME: Here need call errno
        // and set error type.
    }
    return ret;
}

bool SerialPortPrivate::setRts(bool set)
{
    bool ret = trigger_out_line(m_descriptor, TIOCM_RTS, set);
    if (!ret) {
        // FIXME: Here need call errno
        // and set error type.
    }
    return ret;
}

bool SerialPortPrivate::reset()
{
    bool ret = (::tcflush(m_descriptor, TCIOFLUSH) != -1);
    if (!ret) {
        // FIXME: Here need call errno
        // and set error type.
    }
    return ret;
}

bool SerialPortPrivate::sendBreak(int duration)
{
    bool ret = (::tcsendbreak(m_descriptor, duration) != -1);
    if (!ret) {
        // FIXME: Here need call errno
        // and set error type.
    }
    return ret;
}

bool SerialPortPrivate::setBreak(bool set)
{
    bool ret = (::ioctl(m_descriptor, set ? TIOCSBRK : TIOCCBRK) != -1);
    if (!ret) {
        // FIXME: Here need call errno
        // and set error type.
    }
    return ret;
}

qint64 SerialPortPrivate::bytesAvailable() const
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

qint64 SerialPortPrivate::bytesToWrite() const
{
    // FIXME: FIONWRITE (or analogy) is exists?
    return 0;
}

qint64 SerialPortPrivate::read(char *data, qint64 len)
{
    bool sfr = false; // select for read
    bool sfw = false; // select for write
    qint64 bytesReaded = 0;

    do {
        qint64 readFromDevice = ::read(m_descriptor, data, len - bytesReaded);
        if (readFromDevice < 0) {
            bytesReaded = readFromDevice;
            break;
        }
        bytesReaded += readFromDevice;

    } while ((m_dataInterval > 0)
             && (waitForReadOrWrite(m_dataInterval, true, false, &sfr, &sfw) > 0)
             && (sfr)
             && (bytesReaded < len));

    if (bytesReaded < 0) {
        bytesReaded = -1;
        switch (errno) {
#if EWOULDBLOCK-0 && EWOULDBLOCK != EAGAIN
        case EWOULDBLOCK:
#endif
        case EAGAIN:
            // No data was available for reading
            bytesReaded = -2;
            break;
        case EBADF:
        case EINVAL:
        case EIO:
            break;
#ifdef Q_OS_SYMBIAN
        case EPIPE:
#endif
        case ECONNRESET:
#if defined (Q_OS_VXWORKS)
        case ESHUTDOWN:
#endif
            bytesReaded = 0;
            break;
        default:;
        }
        // FIXME: Here need call errno
        // and set error type?
        setError(SerialPort::IoError);
    }
    return bytesReaded;
}

qint64 SerialPortPrivate::write(const char *data, qint64 len)
{
    qint64 bytesWritten = ::write(m_descriptor, data, len);
    nativeFlush();

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
        setError(SerialPort::IoError);
    }
    return bytesWritten;
}

bool SerialPortPrivate::waitForReadOrWrite(int timeout,
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
        *selectForRead = false;
        *selectForWrite = false;
        return false;
    }

    *selectForRead = FD_ISSET(m_descriptor, &fdread);
    *selectForWrite = FD_ISSET(m_descriptor, &fdwrite);
    return true;
}

/* Protected methods */

static const QString defaultPathPrefix = "/dev/";

QString SerialPortPrivate::nativeToSystemLocation(const QString &port) const
{
    QString ret;
    if (!port.contains(defaultPathPrefix))
        ret.append(defaultPathPrefix);
    ret.append(port);
    return ret;
}

QString SerialPortPrivate::nativeFromSystemLocation(const QString &location) const
{
    QString ret = location;
    if (ret.contains(defaultPathPrefix))
        ret.remove(defaultPathPrefix);
    return ret;
}

bool SerialPortPrivate::setNativeRate(qint32 rate, SerialPort::Directions dir)
{
    // Impl me
    return false;
}

bool SerialPortPrivate::setNativeDataBits(SerialPort::DataBits dataBits)
{
    // Impl me
    return false;
}

bool SerialPortPrivate::setNativeParity(SerialPort::Parity parity)
{
    // Impl me
    return false;
}

bool SerialPortPrivate::setNativeStopBits(SerialPort::StopBits stopBits)
{
    // Impl me
    return false;
}

bool SerialPortPrivate::setNativeFlowControl(SerialPort::FlowControl flow)
{
    // Impl me
    return false;
}

bool SerialPortPrivate::setNativeDataInterval(int usecs)
{
    // Impl me
    return false;
}

bool SerialPortPrivate::setNativeReadTimeout(int msecs)
{
    // Impl me
    return false;
}

bool SerialPortPrivate::setNativeDataErrorPolicy(SerialPort::DataErrorPolicy policy)
{
    // Impl me
    return false;
}

bool SerialPortPrivate::nativeFlush()
{
    bool ret = (::tcdrain(m_descriptor) != -1);
    if (!ret) {
        // FIXME: Here need call errno
        // and set error type.
    }
    return ret;
}

void SerialPortPrivate::detectDefaultSettings()
{
    // Impl me
}

// Used only in method SerialPortPrivate::open().
bool SerialPortPrivate::saveOldsettings()
{
    if (::tcgetattr(m_descriptor, &m_oldTermios) == -1)
        return false;

    ::memcpy(&m_currTermios, &m_oldTermios, sizeof(struct termios));
    m_oldSettingsIsSaved = true;
    return true;
}

// Used only in method SerialPortPrivate::close().
bool SerialPortPrivate::restoreOldsettings()
{
    bool restoreResult = true;
    if (m_oldSettingsIsSaved) {
        m_oldSettingsIsSaved = false;
        restoreResult = (::tcsetattr(m_descriptor, TCSANOW, &m_oldTermios) != -1);
    }
    return restoreResult;
}


/* Private methods */


void SerialPortPrivate::prepareTimeouts(int msecs)
{
    // FIXME:
    Q_UNUSED(msecs)

}

inline bool SerialPortPrivate::updateTermious()
{
    return (::tcsetattr(m_descriptor, TCSANOW, &m_currTermios) != -1);
}

bool SerialPortPrivate::setStandartRate(SerialPort::Directions dir, speed_t rate)
{
    // Impl me
    return false;
}

bool SerialPortPrivate::setCustomRate(qint32 rate)
{
#if defined (Q_OS_LINUX)
    //
#else
    //
#endif
    // Impl me
    return false;
}

// Prepares other parameters of the structures port configuration.
// Used only in method SerialPortPrivate::open().
void SerialPortPrivate::prepareOtherOptions()
{
    ::cfmakeraw(&m_currTermios);
    m_currTermios.c_cflag |= (CREAD | CLOCAL);
}



