/*
    License...
*/

#include "serialport_p.h"
#include "ttylocker_p_unix.h"

#include <errno.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#if defined (Q_OS_LINUX)
#  include <linux/serial.h>
#endif

#include <QtCore/QRegExp>


/* Public methods */


SerialPortPrivate::SerialPortPrivate()
    : AbstractSerialPortPrivate()
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
    bool byCurrPid = false;
    if (TTYLocker::isLocked(m_systemLocation, &byCurrPid)) {
        setError(SerialPort::PermissionDeniedError);
        return false;
    }

    int flags = (O_NOCTTY | O_NDELAY);

    switch (mode & QIODevice::ReadWrite) {
    case QIODevice::WriteOnly:
        flags |= O_WRONLY; break;
    case QIODevice::ReadWrite:
        flags |= O_RDWR; break;
    default:
        flags |= O_RDONLY; break;
    }

    // Try opened serial device.
    m_descriptor = ::open(m_systemLocation.toLocal8Bit().constData(), flags);

    if (m_descriptor == -1) {
        switch (errno) {
        case ENODEV:
            setError(SerialPort::NoSuchDeviceError); break;
        case EACCES:
            setError(SerialPort::PermissionDeniedError); break;
        default:
            setError(SerialPort::UnknownPortError);
        }
        return false;
    }

    // Try lock device by location and check it state is locked.
    TTYLocker::lock(m_systemLocation);
    if (!TTYLocker::isLocked(m_systemLocation, &byCurrPid)) {
        setError(SerialPort::PermissionDeniedError);
        return false;
    }

    // Try set exclusive mode.
#if defined (TIOCEXCL)
    ::ioctl(m_descriptor, TIOCEXCL);
#endif

    if (saveOldsettings()) {

        prepareOtherOptions();
        prepareTimeouts(0);

        if (updateTermious()) {
            // Disable autocalculate total read interval.
            // isAutoCalcReadTimeoutConstant = false;

            detectDefaultSettings();
            return true;
        }
    }
    setError(SerialPort::ConfiguringError);
    return false;
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
    bool byCurrPid = false;
    if (TTYLocker::isLocked(m_systemLocation, &byCurrPid) && byCurrPid)
        TTYLocker::unlock(m_systemLocation);

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
    bool sfr = false; // Select for read.
    bool sfw = false; // Select for write.
    qint64 bytesReaded = 0;

    do {
        qint64 readFromDevice = 0;
#if defined (CMSPAR)
        readFromDevice = ::read(m_descriptor, data, len - bytesReaded);
#else
        if ((m_parity != SerialPort::MarkParity) && (m_parity != SerialPort::SpaceParity))
            readFromDevice = ::read(m_descriptor, data, len - bytesReaded);
        else // Perform parity emulation.
            readFromDevice = readPerChar(data, len - bytesReaded);
#endif

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
            // No data was available for reading.
            bytesReaded = -2;
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
    qint64 bytesWritten = 0;
#if defined (CMSPAR)
    bytesWritten = ::write(m_descriptor, data, len);
#else
    if ((m_parity != SerialPort::MarkParity) && (m_parity != SerialPort::SpaceParity))
        bytesWritten = ::write(m_descriptor, data, len);
    else // Perform parity emulation.
        bytesWritten = writePerChar(data, len);
#endif

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

// Returned -1 if rate it is custom baud
// otherwise returned unix speed as speed_t.
static qint32 detect_standard_rate(qint32 rate)
{
    switch (rate) {
#if defined (B0)
    case 0: return B0;
#endif
#if defined (B50)
    case 50: return B50;
#endif
#if defined (B75)
    case 75: return B75;
#endif
#if defined (B110)
    case 110: return B110;
#endif
#if defined (B134)
    case 134: return B134;
#endif
#if defined (B150)
    case 150: return B150;
#endif
#if defined (B200)
    case 200: return B200;
#endif
#if defined (B300)
    case 300: return B300;
#endif
#if defined (B600)
    case 600: return B600;
#endif
#if defined (B1200)
    case 1200: return B1200;
#endif
#if defined (B1800)
    case 1800: return B1800;
#endif
#if defined (B2400)
    case 2400: return B2400;
#endif
#if defined (B4800)
    case 4800: return B4800;
#endif
#if defined (B9600)
    case 9600: return B9600;
#endif
#if defined (B19200)
    case 19200: return B19200;
#endif
#if defined (B38400)
    case 38400: return B38400;
#endif
#if defined (B57600)
    case 57600: return B57600;
#endif
#if defined (B115200)
    case 115200: return B115200;
#endif
#if defined (B230400)
    case 230400: return B230400;
#endif
#if defined (B460800)
    case 460800: return B460800;
#endif
#if defined (B500000)
    case 500000: return B500000;
#endif
#if defined (B576000)
    case 576000: return B576000;
#endif
#if defined (B921600)
    case 921600: return B921600;
#endif
#if defined (B1000000)
    case 1000000: return B1000000;
#endif
#if defined (B1152000)
    case 1152000: return B1152000;
#endif
#if defined (B1500000)
    case 1500000: return B1500000;
#endif
#if defined (B2000000)
    case 2000000: return B2000000;
#endif
#if defined (B2500000)
    case 2500000: return B2500000;
#endif
#if defined (B3000000)
    case 3000000: return B3000000;
#endif
#if defined (B3500000)
    case 3500000: return B3500000;
#endif
#if defined (B4000000)
    case 4000000: return B4000000;
#endif
    default: return -1;
    }
}

bool SerialPortPrivate::setNativeRate(qint32 rate, SerialPort::Directions dir)
{
    if (rate == SerialPort::UnknownRate) {
        setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }

    qint32 detectedRate = detect_standard_rate(rate);
    bool ret = false;
    if (detectedRate == -1)
        ret = setCustomRate(rate);
    else
        ret = setStandartRate(dir, detectedRate);

    if (!ret)
        setError(SerialPort::ConfiguringError);
    return ret;
}

bool SerialPortPrivate::setNativeDataBits(SerialPort::DataBits dataBits)
{
    if ((dataBits == SerialPort::UnknownDataBits)
            || isRestrictedAreaSettings(dataBits, m_stopBits)) {

        setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }

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
        setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }
    bool ret = updateTermious();
    if (!ret)
        setError(SerialPort::ConfiguringError);
    return ret;
}

bool SerialPortPrivate::setNativeParity(SerialPort::Parity parity)
{
    if (parity == SerialPort::UnknownParity) {
        setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }

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
#else
#endif //CMSPAR

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
        m_currTermios.c_iflag &= ~(PARMRK | INPCK);
        m_currTermios.c_iflag |= IGNPAR;

        m_currTermios.c_cflag |= PARENB;
        m_currTermios.c_iflag |= (PARMRK | INPCK);
        m_currTermios.c_iflag &= ~IGNPAR;
        break;
    }

    bool ret = updateTermious();
    if (!ret)
        setError(SerialPort::ConfiguringError);
    return ret;
}

bool SerialPortPrivate::setNativeStopBits(SerialPort::StopBits stopBits)
{
    if ((stopBits == SerialPort::UnknownStopBits)
            || isRestrictedAreaSettings(m_dataBits, stopBits)) {

        setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }

    switch (stopBits) {
    case SerialPort::OneStop:
        m_currTermios.c_cflag &= (~CSTOPB);
        break;
    case SerialPort::TwoStop:
        m_currTermios.c_cflag |= CSTOPB;
        break;
    default:
        setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }
    bool ret = updateTermious();
    if (!ret)
        setError(SerialPort::ConfiguringError);
    return ret;
}

bool SerialPortPrivate::setNativeFlowControl(SerialPort::FlowControl flow)
{
    if (flow == SerialPort::UnknownFlowControl) {
        setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }

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
        setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }
    bool ret = updateTermious();
    if (!ret)
        setError(SerialPort::ConfiguringError);
    return ret;
}

bool SerialPortPrivate::setNativeDataInterval(int usecs)
{
    Q_UNUSED(usecs)
    // Impl me
    return false;
}

bool SerialPortPrivate::setNativeReadTimeout(int msecs)
{
    Q_UNUSED(msecs)
    // Impl me
    return false;
}

bool SerialPortPrivate::setNativeDataErrorPolicy(SerialPort::DataErrorPolicy policy)
{
    Q_UNUSED(policy)
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

static qint32 unixrate2valuerate(speed_t unixrate)
{
    qint32 ret = SerialPort::UnknownRate;

    switch (unixrate) {
#if defined (B50)
    case B50: ret = 50; break;
#endif
#if defined (B75)
    case B75: ret =  75; break;
#endif
#if defined (B110)
    case B110: ret =  110; break;
#endif
#if defined (B134)
    case B134: ret =  134; break;
#endif
#if defined (B150)
    case B150: ret =  150; break;
#endif
#if defined (B200)
    case B200: ret =  200; break;
#endif
#if defined (B300)
    case B300: ret =  300; break;
#endif
#if defined (B600)
    case B600: ret =  600; break;
#endif
#if defined (B1200)
    case B1200: ret =  1200; break;
#endif
#if defined (B1800)
    case B1800: ret =  1800; break;
#endif
#if defined (B2400)
    case B2400: ret =  2400; break;
#endif
#if defined (B4800)
    case B4800: ret =  4800; break;
#endif
#if defined (B9600)
    case B9600: ret =  9600; break;
#endif
#if defined (B19200)
    case B19200: ret =  19200; break;
#endif
#if defined (B38400)
    case B38400: ret =  38400; break;
#endif
#if defined (B57600)
    case B57600: ret =  57600; break;
#endif
#if defined (B115200)
    case B115200: ret =  115200;  break;
#endif
#if defined (B230400)
    case B230400: ret =  230400;  break;
#endif
#if defined (B460800)
    case B460800: ret =  460800;  break;
#endif
#if defined (B500000)
    case B500000: ret =  500000;  break;
#endif
#if defined (B576000)
    case B576000: ret =  576000;  break;
#endif
#if defined (B921600)
    case B921600: ret =  921600;  break;
#endif
#if defined (B1000000)
    case B1000000: ret =  1000000;  break;
#endif
#if defined (B1152000)
    case B1152000: ret =  1152000;  break;
#endif
#if defined (B1500000)
    case B1500000: ret =  1500000;  break;
#endif
#if defined (B2000000)
    case B2000000: ret =  2000000;  break;
#endif
#if defined (B2500000)
    case B2500000: ret =  2500000;  break;
#endif
#if defined (B3000000)
    case B3000000: ret =  3000000;  break;
#endif
#if defined (B3500000)
    case B3500000: ret =  3500000;  break;
#endif
#if defined (B4000000)
    case B4000000: ret =  4000000;  break;
#endif
    default:;
    }
    return ret;
}

void SerialPortPrivate::detectDefaultSettings()
{
    // Detect rate.
    m_inRate = unixrate2valuerate(::cfgetispeed(&m_currTermios));
    m_outRate = unixrate2valuerate(::cfgetospeed(&m_currTermios));

    // Detect databits.
    switch (m_currTermios.c_cflag & CSIZE) {
    case CS5:
        m_dataBits = SerialPort::Data5; break;
    case CS6:
        m_dataBits = SerialPort::Data6; break;
    case CS7:
        m_dataBits = SerialPort::Data7; break;
    case CS8:
        m_dataBits = SerialPort::Data8; break;
    default:
        m_dataBits = SerialPort::UnknownDataBits;
    }

    // Detect parity.
#if defined (CMSPAR)
    if (m_currTermios.c_cflag & CMSPAR) {
        m_parity = (m_currTermios.c_cflag & PARODD) ?
                    SerialPort::MarkParity : SerialPort::SpaceParity;
    } else {
#endif
        if (m_currTermios.c_cflag & PARENB) {
            m_parity = (m_currTermios.c_cflag & PARODD) ?
                        SerialPort::OddParity : SerialPort::EvenParity;
        } else
            m_parity = SerialPort::NoParity;
#if defined (CMSPAR)
    }
#endif

    // Detect stopbits.
    m_stopBits = (m_currTermios.c_cflag & CSTOPB) ?
                SerialPort::TwoStop : SerialPort::OneStop;

    // Detect flow control.
    if ((!(m_currTermios.c_cflag & CRTSCTS)) && (!(m_currTermios.c_iflag & (IXON | IXOFF | IXANY))))
        m_flow = SerialPort::NoFlowControl;
    else if ((!(m_currTermios.c_cflag & CRTSCTS)) && (m_currTermios.c_iflag & (IXON | IXOFF | IXANY)))
        m_flow = SerialPort::SoftwareControl;
    else if ((m_currTermios.c_cflag & CRTSCTS) && (!(m_currTermios.c_iflag & (IXON | IXOFF | IXANY))))
        m_flow = SerialPort::HardwareControl;
    else
        m_flow = SerialPort::UnknownFlowControl;
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
    if (((dir & SerialPort::Input) && (::cfsetispeed(&m_currTermios, rate) == -1))
            || ((dir & SerialPort::Output) && (::cfsetospeed(&m_currTermios, rate) == -1))) {
        return false;
    }
    return updateTermious();
}

bool SerialPortPrivate::setCustomRate(qint32 rate)
{
    int result = -1;
#if defined (Q_OS_LINUX) && defined (TIOCGSERIAL) && defined (TIOCSSERIAL)
    if (rate > 0) {
        struct serial_struct ser_info;
        result = ::ioctl(m_descriptor, TIOCGSERIAL, &ser_info);
        if (result != -1) {
            ser_info.flags &= ~ASYNC_SPD_MASK;
            ser_info.flags |= (ASYNC_SPD_CUST /* | ASYNC_LOW_LATENCY*/);
            ser_info.custom_divisor = ser_info.baud_base / rate;
            if (ser_info.custom_divisor)
                result = ::ioctl(m_descriptor, TIOCSSERIAL, &ser_info);
        }
    }
#else
    //
#endif
    return (result != -1);
}

// Prepares other parameters of the structures port configuration.
// Used only in method SerialPortPrivate::open().
void SerialPortPrivate::prepareOtherOptions()
{
    ::cfmakeraw(&m_currTermios);
    m_currTermios.c_cflag |= (CREAD | CLOCAL);
}

bool SerialPortPrivate::isRestrictedAreaSettings(SerialPort::DataBits dataBits,
                                                 SerialPort::StopBits stopBits) const
{
    return (((dataBits == SerialPort::Data5) && (stopBits == SerialPort::TwoStop))
            || ((dataBits == SerialPort::Data6) && (stopBits == SerialPort::OneAndHalfStop))
            || ((dataBits == SerialPort::Data7) && (stopBits == SerialPort::OneAndHalfStop))
            || ((dataBits == SerialPort::Data8) && (stopBits == SerialPort::OneAndHalfStop)));
}

#if !defined (CMSPAR)
static inline bool evenParity(quint8 c)
{
    // Result:
    // bit4 = bit4^bit5^bit6^bit7
    // bit0 = bit0^bit1^bit2^bit3
    c ^= (c >> 1) ^ (c >> 2) ^ (c >> 3);
    // Result: bit4 ^ bit0
    return (c ^ (c >> 4)) & 1;
}

qint64 SerialPortPrivate::writePerChar(const char *data, qint64 maxSize)
{
    qint64 ret = 0;
    quint8 const charMask = (0xFF >> (8 - m_dataBits));

    while (ret < maxSize) {

        bool par = evenParity(*data & charMask);
        // False if need EVEN, true if need ODD.
        par ^= (m_parity == SerialPort::MarkParity);
        if (par ^ bool(m_currTermios.c_cflag & PARODD)) { // Need switch parity mode?
            m_currTermios.c_cflag ^= PARODD;
            nativeFlush(); //??
            updateTermious();
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

qint64 SerialPortPrivate::readPerChar(char *data, qint64 maxSize)
{
    qint64 ret = 0;
    //quint8 const charMask = (0xFF >> (8 - m_dataBits));

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
        // par ^= evenParity(*data & charMask); //par contains parity bit value for EVEN mode
        // par ^= bool(tio.c_cflag & PARODD); //par contains parity bit value for current mode
        // bool parity_error = (par ^ bool(parity == AbstractSerial::ParitySpace));
        ++data;
        ++ret;
    }
    return ret;
}

#endif //CMSPAR

