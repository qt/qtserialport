/*
    License...
*/

/*!
    \class SymbianSerialPortEngine
    \internal

    \brief The SymbianSerialPortEngine class provides *nix OS
    platform-specific low level access to a serial port.

    \reentrant
    \ingroup serial
    \inmodule QSerialDevice

    Currently the class supports all?? version of Symbian OS.

    SymbianSerialPortEngine (as well as other platform-dependent engines)
    is a class with multiple inheritance, which on the one hand,
    derives from a general abstract class interface SerialPortEngine,
    on the other hand of a class inherited from QObject.

    From the abstract class SerialPortEngine, it inherits all virtual
    interface methods that are common to all serial ports on any platform.
    These methods, the class SymbianSerialPortEngine implements use
    Symbian API.

    From QObject-like class ...
    ...
    ...
    ...

    That is, as seen from the above, the functional SymbianSerialPortEngine
    completely covers all the necessary tasks.
*/

#include "serialportengine_p_symbian.h"

#include <e32base.h>
//#include <e32test.h>
#include <f32file.h>

#include <QtCore/qregexp.h>
//#include <QtCore/QDebug>


// Physical device driver.
#if defined (__WINS__)
_LIT(KPddName, "ECDRV");
#else // defined (__EPOC32__)
_LIT(KPddName, "EUART");
#endif

// Logical  device driver.
_LIT(KLddName,"ECOMM");

// Modules names.
_LIT(KRS232ModuleName, "ECUART");
_LIT(KBluetoothModuleName, "BTCOMM");
_LIT(KInfraRedModuleName, "IRCOMM");
_LIT(KACMModuleName, "ECACM");


// Return false on error load.
static bool loadDevices()
{
    TInt r = KErrNone;
#if defined (__WINS__)
    RFs fileServer;
    r = User::LeaveIfError(fileServer.Connect());
    if (r != KErrNone)
        return false;
    fileServer.Close ();
#endif

    r = User::LoadPhysicalDevice(KPddName);
    if ((r != KErrNone) && (r != KErrAlreadyExists))
        return false; //User::Leave(r);

    r = User::LoadLogicalDevice(KLddName);
    if ((r != KErrNone) && (r != KErrAlreadyExists))
        return false; //User::Leave(r);

#if !defined (__WINS__)
    r = StartC32();
    if ((r != KErrNone) && (r != KErrAlreadyExists))
        return false; //User::Leave(r);
#endif

    return true;
}

QT_USE_NAMESPACE

/* Public methods */

/*!
    Constructs a SymbianSerialPortEngine with \a parent and
    initializes all the internal variables of the initial values.

    A pointer \a parent to the object class SerialPortPrivate
    is required for the recursive call some of its methods.
*/
SymbianSerialPortEngine::SymbianSerialPortEngine(SerialPortPrivate *parent)
{
    Q_ASSERT(parent);
    // Impl me
    m_parent = parent;
}

/*!
    Destructs a SymbianSerialPortEngine,
*/
SymbianSerialPortEngine::~SymbianSerialPortEngine()
{

}

/*!
    Tries to open the object desired serial port by \a location
    in the given open \a mode. In the API of Symbian there is no flag
    to open the port in r/o, w/o or r/w, most likely he always opens
    as r/w.

    Since the port in the Symbian OS can be open in any access mode,
    then this method forcibly puts a port in exclusive mode access.
    In the process of discovery, always set a port in non-blocking
    mode (when the read operation returns immediately) and tries to
    determine its current configuration and install them.

    If successful, returns true; otherwise false, with the setup a
    error code.
*/
bool SymbianSerialPortEngine::open(const QString &location, QIODevice::OpenMode mode)
{
    // Maybe need added check an ReadWrite open mode?
    Q_UNUSED(mode)

    if (!loadDevices()) {
        m_parent->setError(SerialPort::UnknownPortError);
        return false;
    }

    RCommServ server;
    TInt r = server.Connect();
    if (r != KErrNone) {
        m_parent->setError(SerialPort::UnknownPortError);
        return false;
    }

    if (location.contains("BTCOMM"))
        r = server.LoadCommModule(KBluetoothModuleName);
    else if (location.contains("IRCOMM"))
        r = server.LoadCommModule(KInfraRedModuleName);
    else if (location.contains("ACM"))
        r = server.LoadCommModule(KACMModuleName);
    else
        r = server.LoadCommModule(KRS232ModuleName);

    if (r != KErrNone) {
        m_parent->setError(SerialPort::UnknownPortError);
        return false;
    }

    // In Symbian OS port opening only in R/W mode !?
    TPtrC portName(static_cast<const TUint16*>(location.utf16()), location.length());
    r = m_descriptor.Open(server, portName, ECommExclusive);

    if (r != KErrNone) {
        switch (r) {
        case KErrPermissionDenied:
            m_parent->setError(SerialPort::NoSuchDeviceError); break;
        case KErrLocked:
        case KErrAccessDenied:
            m_parent->setError(SerialPort::PermissionDeniedError); break;
        default:
            m_parent->setError(SerialPort::UnknownPortError);
        }
        return false;
    }

    // Save current port settings.
    r = m_descriptor.Config(m_oldSettings);
    if (r != KErrNone) {
        m_parent->setError(SerialPort::UnknownPortError);
        return false;
    }

    detectDefaultSettings();
    return true;
}

/*!
    Closes a serial port object. Before closing restore previous
    serial port settings if necessary.
*/
void SymbianSerialPortEngine::close(const QString &location)
{
    Q_UNUSED(location);

    if (m_parent->m_restoreSettingsOnClose) {
        m_descriptor.SetConfig(m_oldSettings);
    }

    m_descriptor.Close();
}

/*!
    Returns a bitmap state of RS-232 line signals. On error,
    bitmap will be empty (equal zero).

    Symbian API allows you to receive only the state of signals:
    CTS, DSR, DCD, RING, RTS, DTR. Other signals are not available.
*/
SerialPort::Lines SymbianSerialPortEngine::lines() const
{
    SerialPort::Lines ret = 0;

    TUint signalMask = 0;
    m_descriptor.Signals(signalMask);

    if (signalMask & KSignalCTS)
        ret |= SerialPort::Cts;
    if (signalMask & KSignalDSR)
        ret |= SerialPort::Dsr;
    if (signalMask & KSignalDCD)
        ret |= SerialPort::Dcd;
    if (signalMask & KSignalRNG)
        ret |= SerialPort::Ri;
    if (signalMask & KSignalRTS)
        ret |= SerialPort::Rts;
    if (signalMask & KSignalDTR)
        ret |= SerialPort::Dtr;

    //if (signalMask & KSignalBreak)
    //  ret |=
    return ret;
}

/*!
    Set DTR signal to state \a set.


*/
bool SymbianSerialPortEngine::setDtr(bool set)
{
    TInt r;
    if (set)
        r = m_descriptor.SetSignalsToMark(KSignalDTR);
    else
        r = m_descriptor.SetSignalsToSpace(KSignalDTR);

    return (r == KErrNone);
}

/*!
    Set RTS signal to state \a set.


*/
bool SymbianSerialPortEngine::setRts(bool set)
{
    TInt r;
    if (set)
        r = m_descriptor.SetSignalsToMark(KSignalRTS);
    else
        r = m_descriptor.SetSignalsToSpace(KSignalRTS);

    return (r == KErrNone);
}

/*!

*/
bool SymbianSerialPortEngine::flush()
{
    // Impl me
    return false;
}

/*!
    Resets the transmit and receive serial port buffers
    independently.
*/
bool SymbianSerialPortEngine::reset()
{
    TInt r = m_descriptor.ResetBuffers(KCommResetRx | KCommResetTx);
    return (r == KErrNone);
}

/*!
    Sets a break condition for a specified time \a duration
    in milliseconds.

    A break condition on a line is when a data line is held
    permanently high for an indeterminate period which must be
    greater than the time normally taken to transmit two characters.
    It is sometimes used as an error signal between computers and
    other devices attached to them over RS232 lines.

    Setting breaks is not supported on the integral ARM
    serial hardware. EPOC has no support for detecting received
    breaks. There is no way to detects whether setting a break is
    supported using Caps().
*/
bool SymbianSerialPortEngine::sendBreak(int duration)
{
    TRequestStatus status;
    m_descriptor.Break(status, TTimeIntervalMicroSeconds32(duration * 1000));
    return false;
}

/*!

*/
bool SymbianSerialPortEngine::setBreak(bool set)
{
    // Impl me
    return false;
}

/*!
    Gets the number of bytes currently waiting in the
    driver's input buffer. A return value of zero means
    the buffer is empty.
*/
qint64 SymbianSerialPortEngine::bytesAvailable() const
{
    return qint64(m_descriptor.QueryReceiveBuffer());
}

/*!

    It is not possible to find out exactly how many bytes are
    currently in the driver's output buffer waiting to be
    transmitted. However, this is not an issue since it is easy
    to ensure that the output buffer is empty. If the
    KConfigWriteBufferedComplete bit (set via the TCommConfigV01
    structure's iHandshake field) is clear, then all write
    requests will delay completion until the data has completely
    cleared the driver's output buffer.
    If the KConfigWriteBufferedComplete bit is set, a write of zero
    bytes to a port which has data in the output buffer is guaranteed
    to delay completion until the buffer has been fully drained.

*/
qint64 SymbianSerialPortEngine::bytesToWrite() const
{
    // Impl me
    return 0;
}

/*!

    Reads data from a serial port only if it arrives before a
    specified time-out (zero). All reads from the serial device
    use 8-bit descriptors as data buffers, even on a Unicode system.

    The length of the TDes8 is set to zero on entry, which means that
    buffers can be reused without having to be zeroed first.

    The number of bytes to read is set to the maximum length of the
    descriptor.

    If a read is issued with a data length of zero the Read() completes
    immediately but with the side effect that the serial hardware is
    powered up.

    When a Read() terminates with KErrTimedOut, different protocol
    modules can show different behaviours. Some may write any data
    received into the aDes buffer, while others may return just an
    empty descriptor. In the case of a returned empty descriptor use
    ReadOneOrMore() to read any data left in the buffer.

    The behaviour of this API after a call to NotifyDataAvailable() is
    not prescribed and so different CSY's behave differently. IrComm
    will allow a successful completion of this API after a call to
    NotifyDataAvailable(), while ECUART and ECACM will complete the
    request with KErrInUse.

*/
qint64 SymbianSerialPortEngine::read(char *data, qint64 len)
{
    TPtr8 buffer((TUint8 *)data, (int)len);
    TRequestStatus status;
    m_descriptor.Read(status, TTimeIntervalMicroSeconds32(0), buffer);
    User::WaitForRequest(status);
    TInt err = status.Int();
    if (err != KErrNone) {
        m_parent->setError(SerialPort::IoError);
        return qint64(-1);
    }
    return qint64(buffer.Length());
}

/*!

    Writes data to a serial port. All writes to the serial device
    use 8-bit descriptors as data buffers, even on a Unicode system.

    The number of bytes to write is set to the maximum length of
    the descriptor.

    When a Write() is issued with a data length of zero it cannot
    complete until the current handshaking configuration and the
    state of input control lines indicate that it is possible for
    data to be immediately written to the serial line, even though no
    data is to be written. This functionality is useful when
    determining when serial devices come on line, and checking that
    the output buffer is empty (if the KConfigWriteBufferedComplete
    bit is set).

*/
qint64 SymbianSerialPortEngine::write(const char *data, qint64 len)
{
    TPtrC8 buffer((TUint8*)data, (int)len);
    TRequestStatus status;
    m_descriptor.Write(status, buffer);
    User::WaitForRequest(status);
    TInt err = status.Int();

    if (err != KErrNone) {
        m_parent->setError(SerialPort::IoError);
        len = -1;
    }
    // FIXME: How to get the actual number of bytes written?
    return qint64(len);
}

/*!

*/
bool SymbianSerialPortEngine::select(int timeout,
                                     bool checkRead, bool checkWrite,
                                     bool *selectForRead, bool *selectForWrite)
{

    // FIXME: I'm not sure in implementation this method.
    // Someone needs to check and correct.

    TRequestStatus timerStatus;
    TRequestStatus readStatus;
    TRequestStatus writeStatus;

    if (timeout > 0)  {
        if (!m_selectTimer.Handle()) {
            if (m_selectTimer.CreateLocal() != KErrNone)
                return false;
        }
        m_selectTimer.HighRes(timerStatus, timeout * 1000);
    }

    if (checkRead)
        m_descriptor.NotifyDataAvailable(readStatus);

    if (checkWrite)
        m_descriptor.NotifyOutputEmpty(writeStatus);

    enum { STATUSES_COUNT = 3 };
    TRequestStatus *statuses[STATUSES_COUNT];
    TInt num = 0;
    statuses[num++] = &timerStatus;
    statuses[num++] = &readStatus;
    statuses[num++] = &writeStatus;

    User::WaitForNRequest(statuses, num);

    bool result = false;

    // By timeout?
    if (timerStatus != KRequestPending) {
        Q_ASSERT(selectForRead);
        *selectForRead = false;
        Q_ASSERT(selectForWrite);
        *selectForWrite = false;
    } else {
        m_selectTimer.Cancel();
        User::WaitForRequest(timerStatus);

        // By read?
        if (readStatus != KRequestPending) {
            Q_ASSERT(selectForRead);
            *selectForRead = true;
        }

        // By write?
        if (writeStatus != KRequestPending) {
            Q_ASSERT(selectForWrite);
            *selectForWrite = true;
        }

        if (checkRead)
            m_descriptor.NotifyDataAvailableCancel();
        if (checkWrite)
            m_descriptor.NotifyOutputEmptyCancel();

        result = true;
    }
    return result;
}

//static const QString defaultPathPostfix = ":";

/*!
    Converts a platform specific \a port name to system location
    and return result.

    Does not do anything because These concepts are equivalent.
*/
QString SymbianSerialPortEngine::toSystemLocation(const QString &port) const
{
    // Port name is equval to port location.
    return port;
}

/*!
    Converts a platform specific system \a location to port name
    and return result.

    Does not do anything because These concepts are equivalent.
*/
QString SymbianSerialPortEngine::fromSystemLocation(const QString &location) const
{
    // Port name is equval to port location.
    return location;
}

/*!
    Set desired \a rate by given direction \a dir.
    However, Symbian does not support separate directions, so the
    method will return an error. Also it supports only the standard
    set of speed.

    If successful, returns true; otherwise false, with the setup a
    error code.
*/
bool SymbianSerialPortEngine::setRate(qint32 rate, SerialPort::Directions dir)
{
    if (dir != SerialPort::AllDirections) {
        m_parent->setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }

    switch (rate) {
    case 50:
        m_currSettings().iRate = EBps50;
        break;
    case 75:
        m_currSettings().iRate = EBps75;
        break;
    case 110:
        m_currSettings().iRate = EBps110;
        break;
    case 134:
        m_currSettings().iRate = EBps134;
        break;
    case 150:
        m_currSettings().iRate = EBps150;
        break;
    case 300:
        m_currSettings().iRate = EBps300;
        break;
    case 600:
        m_currSettings().iRate = EBps600;
        break;
    case 1200:
        m_currSettings().iRate = EBps1200;
        break;
    case 1800:
        m_currSettings().iRate = EBps1800;
        break;
    case 2000:
        m_currSettings().iRate = EBps2000;
        break;
    case 2400:
        m_currSettings().iRate = EBps2400;
        break;
    case 3600:
        m_currSettings().iRate = EBps3600;
        break;
    case 4800:
        m_currSettings().iRate = EBps4800;
        break;
    case 7200:
        m_currSettings().iRate = EBps7200;
        break;
    case 9600:
        m_currSettings().iRate = EBps9600;
        break;
    case 19200:
        m_currSettings().iRate = EBps19200;
        break;
    case 38400:
        m_currSettings().iRate = EBps38400;
        break;
    case 57600:
        m_currSettings().iRate = EBps57600;
        break;
    case 115200:
        m_currSettings().iRate = EBps115200;
        break;
    case 230400:
        m_currSettings().iRate = EBps230400;
        break;
    case 460800:
        m_currSettings().iRate = EBps460800;
        break;
    case 576000:
        m_currSettings().iRate = EBps576000;
        break;
    case 1152000:
        m_currSettings().iRate = EBps1152000;
        break;
    case 4000000:
        m_currSettings().iRate = EBps4000000;
        break;
    case 921600:
        m_currSettings().iRate = EBps921600;
        break;
        //case 1843200:; // Only for  Symbian SR1
    default:
        m_parent->setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }

    return updateCommConfig();
}

/*!
    Set desired number of data bits \a dataBits in byte. Symbian
    native supported all present number of data bits 5, 6, 7, 8.

    If successful, returns true; otherwise false, with the setup a
    error code.
*/
bool SymbianSerialPortEngine::setDataBits(SerialPort::DataBits dataBits)
{
    switch (dataBits) {
    case SerialPort::Data5:
        m_currSettings().iDataBits = EData5;
        break;
    case SerialPort::Data6:
        m_currSettings().iDataBits = EData6;
        break;
    case SerialPort::Data7:
        m_currSettings().iDataBits = EData7;
        break;
    case SerialPort::Data8:
        m_currSettings().iDataBits = EData8;
        break;
    default:
        m_parent->setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }

    return updateCommConfig();
}

/*!
    Set desired \a parity control mode. Symbian native supported
    all present parity types no parity, space, mark, even, odd.

    If successful, returns true; otherwise false, with the setup a
    error code.
*/
bool SymbianSerialPortEngine::setParity(SerialPort::Parity parity)
{
    switch (parity) {
    case SerialPort::NoParity:
        m_currSettings().iParity = EParityNone;
        break;
    case SerialPort::EvenParity:
        m_currSettings().iParity = EParityEven;
        break;
    case SerialPort::OddParity:
        m_currSettings().iParity = EParityOdd;
        break;
    case SerialPort::MarkParity:
        m_currSettings().iParity = EParityMark;
        break;
    case SerialPort::SpaceParity:
        m_currSettings().iParity = EParitySpace;
        break;
    default:
        m_parent->setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }

    return updateCommConfig();
}

/*!
    Set desired number of stop bits \a stopBits in frame. Symbian
    native supported only 1, 2 number of stop bits.

    If successful, returns true; otherwise false, with the setup a
    error code.
*/
bool SymbianSerialPortEngine::setStopBits(SerialPort::StopBits stopBits)
{
    switch (stopBits) {
    case SerialPort::OneStop:
        m_currSettings().iStopBits = EStop1;
        break;
    case SerialPort::TwoStop:
        m_currSettings().iStopBits = EStop2;
        break;
    default:
        m_parent->setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }

    return updateCommConfig();
}

/*!
    Set desired \a flow control mode. Symbian native supported all
    present flow control modes no control, hardware (RTS/CTS),
    software (XON/XOFF).

    If successful, returns true; otherwise false, with the setup a
    error code.
*/
bool SymbianSerialPortEngine::setFlowControl(SerialPort::FlowControl flow)
{
    switch (flow) {
    case SerialPort::NoFlowControl:
        m_currSettings().iHandshake = KConfigFailDSR;
        break;
    case SerialPort::HardwareControl:
        m_currSettings().iHandshake = KConfigObeyCTS | KConfigFreeRTS;
        break;
    case SerialPort::SoftwareControl:
        m_currSettings().iHandshake = KConfigObeyXoff | KConfigSendXoff;
        break;
    default:
        m_parent->setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }

    return updateCommConfig();
}

/*!

*/
bool SymbianSerialPortEngine::setDataErrorPolicy(SerialPort::DataErrorPolicy policy)
{
    Q_UNUSED(policy)
    // Impl me
    return true;
}

/*!

*/
bool SymbianSerialPortEngine::isReadNotificationEnabled() const
{
    // Impl me
    return false;
}

/*!

*/
void SymbianSerialPortEngine::setReadNotificationEnabled(bool enable)
{
    Q_UNUSED(enable)
    // Impl me
}

/*!

*/
bool SymbianSerialPortEngine::isWriteNotificationEnabled() const
{
    // Impl me
    return false;
}

/*!

*/
void SymbianSerialPortEngine::setWriteNotificationEnabled(bool enable)
{
    Q_UNUSED(enable)
    // Impl me
}

/*!

*/
bool SymbianSerialPortEngine::processIOErrors()
{
    // Impl me
    return false;
}

///

/* Protected methods */

/*!
    Attempts to determine the current settings of the serial port,
    wehn it opened. Used only in the method open().
*/
void SymbianSerialPortEngine::detectDefaultSettings()
{
    // Detect rate.
    switch (m_currSettings().iRate) {
    case EBps50:
        m_parent->m_inRate = 50;
        break;
    case EBps75:
        m_parent->m_inRate = 75;
        break;
    case EBps110:
        m_parent->m_inRate = 110;
        break;
    case EBps134:
        m_parent->m_inRate = 134;
        break;
    case EBps150:
        m_parent->m_inRate = 150;
        break;
    case EBps300:
        m_parent->m_inRate = 300;
        break;
    case EBps600:
        m_parent->m_inRate = 600;
        break;
    case EBps1200:
        m_parent->m_inRate = 1200;
        break;
    case EBps1800:
        m_parent->m_inRate = 1800;
        break;
    case EBps2000:
        m_parent->m_inRate = 2000;
        break;
    case EBps2400:
        m_parent->m_inRate = 2400;
        break;
    case EBps3600:
        m_parent->m_inRate = 3600;
        break;
    case EBps4800:
        m_parent->m_inRate = 4800;
        break;
    case EBps7200:
        m_parent->m_inRate = 7200;
        break;
    case EBps9600:
        m_parent->m_inRate = 9600;
        break;
    case EBps19200:
        m_parent->m_inRate = 19200;
        break;
    case EBps38400:
        m_parent->m_inRate = 38400;
        break;
    case EBps57600:
        m_parent->m_inRate = 57600;
        break;
    case EBps115200:
        m_parent->m_inRate = 115200;
        break;
    case EBps230400:
        m_parent->m_inRate = 230400;
        break;
    case EBps460800:
        m_parent->m_inRate = 460800;
        break;
    case EBps576000:
        m_parent->m_inRate = 576000;
        break;
    case EBps1152000:
        m_parent->m_inRate = 1152000;
        break;
    case EBps4000000:
        m_parent->m_inRate = 4000000;
        break;
    case EBps921600:
        m_parent->m_inRate = 921600;
        break;
        //case EBps1843200: m_inRate = 1843200; break;
    default:
        m_parent->m_inRate = SerialPort::UnknownRate;
    }
    m_parent->m_outRate = m_parent->m_inRate;

    // Detect databits.
    switch (m_currSettings().iDataBits) {
    case EData5:
        m_parent->m_dataBits = SerialPort::Data5;
        break;
    case EData6:
        m_parent->m_dataBits = SerialPort::Data6;
        break;
    case EData7:
        m_parent->m_dataBits = SerialPort::Data7;
        break;
    case EData8:
        m_parent->m_dataBits = SerialPort::Data8;
        break;
    default:
        m_parent->m_dataBits = SerialPort::UnknownDataBits;
    }

    // Detect parity.
    switch (m_currSettings().iParity) {
    case EParityNone:
        m_parent->m_parity = SerialPort::NoParity;
        break;
    case EParityEven:
        m_parent->m_parity = SerialPort::EvenParity;
        break;
    case EParityOdd:
        m_parent->m_parity = SerialPort::OddParity;
        break;
    case EParityMark:
        m_parent->m_parity = SerialPort::MarkParity;
        break;
    case EParitySpace:
        m_parent->m_parity = SerialPort::SpaceParity;
        break;
    default:
        m_parent->m_parity = SerialPort::UnknownParity;
    }

    // Detect stopbits.
    switch (m_currSettings().iStopBits) {
    case EStop1:
        m_parent->m_stopBits = SerialPort::OneStop;
        break;
    case EStop2:
        m_parent->m_stopBits = SerialPort::TwoStop;
        break;
    default:
        m_parent->m_stopBits = SerialPort::UnknownStopBits;
    }

    // Detect flow control.
    if ((m_currSettings().iHandshake & (KConfigObeyXoff | KConfigSendXoff))
            == (KConfigObeyXoff | KConfigSendXoff))
        m_parent->m_flow = SerialPort::SoftwareControl;
    else if ((m_currSettings().iHandshake & (KConfigObeyCTS | KConfigFreeRTS))
             == (KConfigObeyCTS | KConfigFreeRTS))
        m_parent->m_flow = SerialPort::HardwareControl;
    else if (m_currSettings().iHandshake & KConfigFailDSR)
        m_parent->m_flow = SerialPort::NoFlowControl;
    else
        m_parent->m_flow = SerialPort::UnknownFlowControl;
}

/* Private methods */

/*!
    Updates the TCommConfig structure wehn changing of any the
    parameters a serial port.

    If successful, returns true; otherwise false.
*/
bool SymbianSerialPortEngine::updateCommConfig()
{
    if (m_descriptor.SetConfig(m_currSettings) != KErrNone) {
        m_parent->setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }
    return true;
}

// From <serialportengine_p.h>
SerialPortEngine *SerialPortEngine::create(SerialPortPrivate *parent)
{
    return new SymbianSerialPortEngine(parent);
}




