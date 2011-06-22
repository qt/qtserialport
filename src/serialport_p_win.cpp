/*
    License...
*/

#include "serialport_p_win.h"

#include <QtCore/QRegExp>

#ifndef Q_CC_MSVC
#  include <ddk/ntddser.h>
#else
#ifndef IOCTL_SERIAL_GET_DTRRTS
#define IOCTL_SERIAL_GET_DTRRTS \
    CTL_CODE (FILE_DEVICE_SERIAL_PORT, 30, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif //Q_CC_MSVC

#ifndef SERIAL_DTR_STATE
#  define SERIAL_DTR_STATE                  0x00000001
#endif

#ifndef SERIAL_RTS_STATE
#  define SERIAL_RTS_STATE                  0x00000002
#endif

#endif


/* Public */

SerialPortPrivateWin::SerialPortPrivateWin()
    : SerialPortPrivate()
    , m_descriptor(INVALID_HANDLE_VALUE)
{
    size_t size = sizeof(DCB);
    ::memset((void *)(&m_currDCB), 0, size);
    ::memset((void *)(&m_oldDCB), 0, size);
    size = sizeof(COMMTIMEOUTS);
    ::memset((void *)(&m_currCommTimeouts), 0, size);
    ::memset((void *)(&m_oldCommTimeouts), 0, size);
    size = sizeof(OVERLAPPED);
    ::memset((void *)(&m_ovRead), 0, size);
    ::memset((void *)(&m_ovWrite), 0, size);
    ::memset((void *)(&m_ovSelect), 0, size);
}

SerialPortPrivateWin::~SerialPortPrivateWin()
{
}

bool SerialPortPrivateWin::open(QIODevice::OpenMode mode)
{
    DWORD access = 0;
    DWORD sharing = 0;
    bool rxflag = false;
    bool txflag = false;

    if (QIODevice::ReadOnly & mode) {
        access |= GENERIC_READ; //sharing = FILE_SHARE_READ;
        rxflag = true;
    }
    if (QIODevice::WriteOnly & mode) {
        access |= GENERIC_WRITE; //sharing = FILE_SHARE_WRITE;
        txflag = true;
    }

    QByteArray nativeFilePath = QByteArray((const char *)m_systemLocation.utf16(), m_systemLocation.size() * 2 + 1);
    m_descriptor = ::CreateFile((const wchar_t*)nativeFilePath.constData(),
                                access,
                                sharing,
                                0,
                                OPEN_EXISTING,
                                FILE_FLAG_OVERLAPPED,
                                0);

    if (INVALID_HANDLE_VALUE == m_descriptor) {
        //
        return false;
    }

    if (!saveOldsettings()) {
        //
        return false;
    }

    // prepareOtherOptions();

    if (!updateDcb()) {
        //
        return false;
    }

    // Prepare timeouts.
    /*
    prepareCommTimeouts(SerialPortPrivateWin::ReadIntervalTimeout, MAXWORD);
    prepareCommTimeouts(SerialPortPrivateWin::ReadTotalTimeoutMultiplier, 0);
    prepareCommTimeouts(SerialPortPrivateWin::ReadTotalTimeoutConstant, 0);
    prepareCommTimeouts(SerialPortPrivateWin::WriteTotalTimeoutMultiplier, 0);
    prepareCommTimeouts(SerialPortPrivateWin::WriteTotalTimeoutConstant, 0);
    */

    if (!updateCommTimeouts()) {
        //
        return false;
    }

    // Disable autocalculate total read interval.
    //this->isAutoCalcReadTimeoutConstant = false;

    if (!createEvents(rxflag, txflag)) {
        //
        return false;
    }

    detectDefaultSettings();
    return true;
}

void SerialPortPrivateWin::close()
{
    restoreOldsettings();
    ::CancelIo(m_descriptor);
    ::CloseHandle(m_descriptor);
    closeEvents();
    m_descriptor = INVALID_HANDLE_VALUE;
}

bool SerialPortPrivateWin::dtr() const
{
    return (SerialPort::Dtr && lines());
}

bool SerialPortPrivateWin::rts() const
{
    return (SerialPort::Rts && lines());
}

SerialPort::Lines SerialPortPrivateWin::lines() const
{
    DWORD modemStat = 0;
    SerialPort::Lines result = 0;

    if (0 == ::GetCommModemStatus(m_descriptor, &modemStat)) {
        // Print error?
        return result;
    }

    if (modemStat & MS_CTS_ON)
        result |= SerialPort::Cts;
    if (modemStat & MS_DSR_ON)
        result |= SerialPort::Dsr;
    if (modemStat & MS_RING_ON)
        result |= SerialPort::Ri;
    if (modemStat & MS_RLSD_ON)
        result |= SerialPort::Dcd;

    DWORD bytesReturned = 0;
    if (::DeviceIoControl(m_descriptor, IOCTL_SERIAL_GET_DTRRTS, 0, 0,
                          &modemStat, sizeof(DWORD),
                          &bytesReturned, 0)) {

        if (modemStat & SERIAL_DTR_STATE)
            result |= SerialPort::Dtr;
        if (modemStat & SERIAL_RTS_STATE)
            result |= SerialPort::Rts;
    }

    return result;
}

bool SerialPortPrivateWin::flush()
{

}

bool SerialPortPrivateWin::reset()
{

}

qint64 SerialPortPrivateWin::bytesAvailable() const
{

}

qint64 SerialPortPrivateWin::read(char *data, qint64 len)
{

}

qint64 SerialPortPrivateWin::write(const char *data, qint64 len)
{
    //if (!clear_overlapped(&m_ovWrite))
    //    return qint64(-1);

    DWORD writeBytes = 0;
    bool sucessResult = false;

    if (::WriteFile(m_descriptor, (LPCVOID)data, (DWORD)len, &writeBytes, &m_ovWrite))
        sucessResult = true;
    else {
        if (ERROR_IO_PENDING == ::GetLastError()) {
            //not to loop the function, instead of setting INFINITE put 5000 milliseconds wait!
            switch (::WaitForSingleObject(m_ovWrite.hEvent, 5000)) {
            case WAIT_OBJECT_0: {
                if (::GetOverlappedResult(m_descriptor, &m_ovWrite, &writeBytes, false))
                    sucessResult = true;
                else {
                    // Print error?
                    ;
                }
            }
            break;//WAIT_OBJECT_0
            default:
                //Print error ?
                ;
            }//switch (rc)
        }
        else {
#if defined (NATIVESERIALENGINE_WIN_DEBUG)
            qDebug() << "Windows: NativeSerialEnginePrivate::nativeWrite(const char *data, qint64 len) \n"
                        " -> function: ::GetLastError() returned: " << rc << ". Error! \n";
#endif
            ;
        }
    }

    return (sucessResult) ? quint64(writeBytes) : qint64(-1);
}

bool SerialPortPrivateWin::waitForReadyRead(int msec)
{

}

bool SerialPortPrivateWin::waitForBytesWritten(int msec)
{

}

/* Protected */

static const QString defaultPathPrefix = "\\\\.\\";

QString SerialPortPrivateWin::nativeToSystemLocation(const QString &port) const
{
    QString result;
    if (!port.contains(defaultPathPrefix))
        result.append(defaultPathPrefix);
    result.append(port);
    return result;
}

QString SerialPortPrivateWin::nativeFromSystemLocation(const QString &location) const
{
    QString result = location;
    if (result.contains(defaultPathPrefix))
        result.remove(defaultPathPrefix);
    return result;
}

bool SerialPortPrivateWin::setNativeRate(qint32 rate, SerialPort::Directions dir)
{
    if ((SerialPort::UnknownRate == rate)
            || (SerialPort::AllDirections != dir)) {
        return false;
    }
    m_currDCB.BaudRate = DWORD(rate);
    return (updateDcb());
}

bool SerialPortPrivateWin::setNativeDataBits(SerialPort::DataBits dataBits)
{
    if (SerialPort::UnknownDataBits == dataBits)
        return false;

    if ((SerialPort::Data5 == dataBits)
            && (SerialPort::TwoStop == m_stopBits)) {
        //
        return false;
    }
    if ((SerialPort::Data6 == dataBits)
            && (SerialPort::OneAndHalfStop == m_stopBits)) {
        //
        return false;
    }
    if ((SerialPort::Data7 == dataBits)
            && (SerialPort::OneAndHalfStop == m_stopBits)) {
        //
        return false;
    }
    if ((SerialPort::Data8 == dataBits)
            && (SerialPort::OneAndHalfStop == m_stopBits)) {
        //
        return false;
    }
    m_currDCB.ByteSize = BYTE(dataBits);
    return (updateDcb());
}

bool SerialPortPrivateWin::setNativeParity(SerialPort::Parity parity)
{
    if (SerialPort::UnknownParity == parity)
        return false;

    m_currDCB.fParity = true;
    switch (parity) {
    case SerialPort::NoParity:  {
        m_currDCB.Parity = NOPARITY;
        m_currDCB.fParity = false;
    }
    break;
    case SerialPort::SpaceParity: m_currDCB.Parity = SPACEPARITY; break;
    case SerialPort::MarkParity: m_currDCB.Parity = MARKPARITY; break;
    case SerialPort::EvenParity: m_currDCB.Parity = EVENPARITY; break;
    case SerialPort::OddParity: m_currDCB.Parity = ODDPARITY; break;
    default: return false;
    }
    return (updateDcb());
}

bool SerialPortPrivateWin::setNativeStopBits(SerialPort::StopBits stopBits)
{
    if (SerialPort::UnknownStopBits == stopBits)
        return false;

    if ((SerialPort::Data5 == m_dataBits)
            && (SerialPort::TwoStop == m_stopBits)) {
        return false;
    }
    if ((SerialPort::Data6 == m_dataBits)
            && (SerialPort::OneAndHalfStop == m_stopBits)) {
        return false;
    }
    if ((SerialPort::Data7 == m_dataBits)
            && (SerialPort::OneAndHalfStop == m_stopBits)) {
        return false;
    }
    if ((SerialPort::Data8 == m_dataBits)
            && (SerialPort::OneAndHalfStop == m_stopBits)) {
        return false;
    }

    switch (stopBits) {
    case SerialPort::OneStop: m_currDCB.StopBits = ONESTOPBIT; break;
    case SerialPort::OneAndHalfStop: m_currDCB.StopBits = ONE5STOPBITS; break;
    case SerialPort::TwoStop: m_currDCB.StopBits = TWOSTOPBITS; break;
    default: return false;
    }
    return (updateDcb());
}

bool SerialPortPrivateWin::setNativeFlowControl(SerialPort::FlowControl flow)
{
    if (SerialPort::UnknownFlowControl == flow)
        return false;

    switch (flow) {
    case SerialPort::NoFlowControl: {
        m_currDCB.fOutxCtsFlow = false;
        m_currDCB.fRtsControl = RTS_CONTROL_DISABLE;
        m_currDCB.fInX = m_currDCB.fOutX = false;
    }
    break;
    case SerialPort::SoftwareControl: {
        m_currDCB.fOutxCtsFlow = false;
        m_currDCB.fRtsControl = RTS_CONTROL_DISABLE;
        m_currDCB.fInX = m_currDCB.fOutX = true;
    }
    break;
    case SerialPort::HardwareControl: {
        m_currDCB.fOutxCtsFlow = true;
        m_currDCB.fRtsControl = RTS_CONTROL_HANDSHAKE;
        m_currDCB.fInX = m_currDCB.fOutX = false;
    }
    break;
    default: return false;
    }
    return (updateDcb());
}

bool SerialPortPrivateWin::setNativeDataInterval(int usecs)
{

}

bool SerialPortPrivateWin::setNativeReadTimeout(int msecs)
{

}

bool SerialPortPrivateWin::setNativeDataErrorPolicy(SerialPort::DataErrorPolicy policy)
{

}

bool SerialPortPrivateWin::waitForReadOrWrite(int timeout,
                                       bool checkRead, bool checkWrite,
                                       bool *selectForRead, bool *selectForWrite)
{
    if (checkRead && (bytesAvailable() > 0)) {
        *selectForRead = true;
        return 1;
    }

    //if (!clear_overlapped(&m_ovSelect))
    //    return int(-1);

    DWORD oldEventMask = 0;
    DWORD currEventMask = 0;

    if (checkRead)
        currEventMask |= EV_RXCHAR;
    if (checkWrite)
        currEventMask |= EV_TXEMPTY;

    //save old mask
    if (0 == ::GetCommMask(m_descriptor, &oldEventMask)) {
        //Print error?
        return -1;
    }

    if (currEventMask != (oldEventMask & currEventMask)) {
        currEventMask |= oldEventMask;
        //set mask
        if(0 == ::SetCommMask(m_descriptor, currEventMask)) {
            //Print error?
            return -1;
        }
    }

    currEventMask = 0;
    bool selectResult = false;

    if (::WaitCommEvent(m_descriptor, &currEventMask, &m_ovSelect))
        selectResult = true;
    else {
        if (ERROR_IO_PENDING == ::GetLastError()) {
            DWORD bytesTransferred = 0;
            switch (::WaitForSingleObject(m_ovSelect.hEvent, (timeout < 0) ? 0 : timeout)) {
            case WAIT_OBJECT_0: {
                if (::GetOverlappedResult(m_descriptor, &m_ovSelect, &bytesTransferred, false))
                    selectResult = true;
                else {
                    //Print error?
                    ;
                }
            }
            break;
            case WAIT_TIMEOUT:
                //Print error?
                ;
                break;
            default:
                //Print error?
                ;
            }
        }
        else {
            //Print error?
            ;
        }
    }

    if (selectResult) {
        *selectForRead = (currEventMask & EV_RXCHAR)
                && (checkRead)
                && (bytesAvailable());
        *selectForWrite = (currEventMask & EV_TXEMPTY) && (checkWrite);
    }
    ::SetCommMask(m_descriptor, oldEventMask); //rerair old mask
    return int(selectResult);
}

void SerialPortPrivateWin::detectDefaultSettings()
{

}

// Used only in method SerialPortPrivateWin::open(SerialPort::OpenMode mode).
bool SerialPortPrivateWin::saveOldsettings()
{
    DWORD confSize = sizeof(DCB);
    if (0 == ::GetCommState(m_descriptor, &m_oldDCB))
        return false;
    ::memcpy((void *)(&m_currDCB), (const void *)(&m_oldDCB), confSize);

    confSize = sizeof(COMMTIMEOUTS);
    if (0 == ::GetCommTimeouts(m_descriptor, &m_oldCommTimeouts))
        return false;
    ::memcpy((void *)(&m_currCommTimeouts), (const void *)(&m_oldCommTimeouts), confSize);

    m_oldSettingsIsSaved = true;
    return true;
}

// Used only in method SerialPortPrivateWin::close().
bool SerialPortPrivateWin::restoreOldsettings()
{
    bool restoreResult = true;
    if (m_oldSettingsIsSaved) {
        m_oldSettingsIsSaved = false;
        if (0 != ::GetCommState(m_descriptor, &m_oldDCB))
            restoreResult = false;
        if (0 == ::SetCommTimeouts(m_descriptor, &m_oldCommTimeouts))
            restoreResult = false;
    }
    return restoreResult;
}

/* Private */

bool SerialPortPrivateWin::createEvents(bool rx, bool tx)
{
    if (rx) { m_ovRead.hEvent = ::CreateEvent(0, false, false, 0); }
    if (tx) { m_ovWrite.hEvent = ::CreateEvent(0, false, false, 0); }
    m_ovSelect.hEvent = ::CreateEvent(0, false, false, 0);

    return ((rx && (0 == m_ovRead.hEvent))
            || (tx && (0 == m_ovWrite.hEvent))
            || (0 == m_ovSelect.hEvent)) ? false : true;
}

void SerialPortPrivateWin::closeEvents() const
{
    if (m_ovRead.hEvent)
        ::CloseHandle(m_ovRead.hEvent);
    if (m_ovWrite.hEvent)
        ::CloseHandle(m_ovWrite.hEvent);
    if (m_ovSelect.hEvent)
        ::CloseHandle(m_ovSelect.hEvent);
}

void SerialPortPrivateWin::recalcTotalReadTimeoutConstant()
{

}

void SerialPortPrivateWin::prepareCommTimeouts(CommTimeouts cto, DWORD msecs)
{

}

inline bool SerialPortPrivateWin::updateDcb()
{
    return (0 != ::SetCommState(m_descriptor, &m_currDCB));
}

inline bool SerialPortPrivateWin::updateCommTimeouts()
{
    return (0 != ::SetCommTimeouts(m_descriptor, &m_currCommTimeouts));
}



