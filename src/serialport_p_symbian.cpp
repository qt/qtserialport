/*
    License...
*/

#include "serialport_p.h"

#include <e32base.h>
//#include <e32test.h>
#include <f32file.h>

#include <QtCore/QRegExp>
//#include <QtCore/QDebug>


// Physical device driver.
#if defined (__WINS__)
_LIT(KPddName, "ECDRV");
#else // defined (__EPOC32__)
_LIT(KPddName, "EUART");
#endif

// Logical native device driver.
_LIT(KLddName,"ECOMM");

// Modules names.
_LIT(KRS232ModuleName , "ECUART");
_LIT(KBluetoothModuleName , "BTCOMM");
_LIT(KInfraRedModuleName , "IRCOMM");


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



/* Public methods */


SerialPortPrivate::SerialPortPrivate()
    : AbstractSerialPortPrivate()
{
    // Impl me

    m_notifier.setRef(this);
}

bool SerialPortPrivate::open(QIODevice::OpenMode mode)
{
    Q_UNUSED(mode)

    if (!loadDevices()) {
        setError(SerialPort::UnknownPortError);
        return false;
    }

    RCommServ server;
    TInt r = server.Connect();
    if (r != KErrNone) {
        setError(SerialPort::UnknownPortError);
        return false;
    }

    // Here you need to add automatic detection of loaded module
    // name, depending on the port name.
    r = server.LoadCommModule(KRS232ModuleName);
    if (r != KErrNone) {
        setError(SerialPort::UnknownPortError);
        return false;
    }

    // In Symbian OS port opening only in R/W mode !?
    TPtrC portName(static_cast<const TUint16*>(m_systemLocation.utf16()), m_systemLocation.length());
    r = m_descriptor.Open(server, portName, ECommExclusive);

    if (r != KErrNone) {
        switch (r) {
        case KErrPermissionDenied:
            setError(SerialPort::NoSuchDeviceError); break;
        case KErrLocked:
        case KErrAccessDenied:
            setError(SerialPort::PermissionDeniedError); break;
        default:
            setError(SerialPort::UnknownPortError);
        }
        return false;
    }

    if (saveOldsettings()) {
        detectDefaultSettings();
    }
    setError(SerialPort::ConfiguringError);
    return false;
}

void SerialPortPrivate::close()
{
    restoreOldsettings();
    m_descriptor.Close();
}

SerialPort::Lines SerialPortPrivate::lines() const
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

bool SerialPortPrivate::setDtr(bool set)
{
    if (set)
        m_descriptor.SetSignalsToMark(KSignalDTR);
    else
        m_descriptor.SetSignalsToSpace(KSignalDTR);
    return true;
}

bool SerialPortPrivate::setRts(bool set)
{
    if (set)
        m_descriptor.SetSignalsToMark(KSignalRTS);
    else
        m_descriptor.SetSignalsToSpace(KSignalRTS);
    return true;
}

bool SerialPortPrivate::reset()
{
    m_descriptor.ResetBuffers(KCommResetRx | KCommResetTx);
    return true;
}

bool SerialPortPrivate::sendBreak(int duration)
{
    TRequestStatus status;
    m_descriptor.Break(status, TTimeIntervalMicroSeconds32(duration * 1000));
    return false;
}

bool SerialPortPrivate::setBreak(bool set)
{
    // Impl me
    return false;
}

qint64 SerialPortPrivate::bytesAvailable() const
{
    return qint64(m_descriptor.QueryReceiveBuffer());
}

qint64 SerialPortPrivate::bytesToWrite() const
{
    // Impl me
    return -1;
}

qint64 SerialPortPrivate::read(char *data, qint64 len)
{
    TRequestStatus status;
    //TBuf8< 512 > buf;
    //TPtr8 buf(TUint8(data), TInt(len));

    // m_descriptor.Read(status, TTimeIntervalMicroSeconds32(0), buf);
    // User::WaitForRequest(status);
    //TInt r = status.Int();





    // Impl me
    return -1;
}

qint64 SerialPortPrivate::write(const char *data, qint64 len)
{
    // Impl me
    return -1;
}

bool SerialPortPrivate::waitForReadOrWrite(int timeout,
                                           bool checkRead, bool checkWrite,
                                           bool *selectForRead, bool *selectForWrite)
{
    // Impl me
    return false;
}


/* Protected methods */


static const QString defaultPathPostfix = ":";

QString SerialPortPrivate::nativeToSystemLocation(const QString &port) const
{
    // Port name is equval to port location.
    return port;
}

QString SerialPortPrivate::nativeFromSystemLocation(const QString &location) const
{
    // Port name is equval to port location.
    return location;
}

bool SerialPortPrivate::setNativeRate(qint32 rate, SerialPort::Directions dir)
{
    if ((rate == SerialPort::UnknownRate) || (dir != SerialPort::AllDirections)) {
        setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }

    switch (rate) {
    case 50: m_currSettings().iRate = EBps50; break;
    case 75: m_currSettings().iRate = EBps75; break;
    case 110: m_currSettings().iRate = EBps110; break;
    case 134: m_currSettings().iRate = EBps134; break;
    case 150: m_currSettings().iRate = EBps150; break;
    case 300: m_currSettings().iRate = EBps300; break;
    case 600: m_currSettings().iRate = EBps600; break;
    case 1200: m_currSettings().iRate = EBps1200; break;
    case 1800: m_currSettings().iRate = EBps1800; break;
    case 2000: m_currSettings().iRate = EBps2000; break;
    case 2400: m_currSettings().iRate = EBps2400; break;
    case 3600: m_currSettings().iRate = EBps3600; break;
    case 4800: m_currSettings().iRate = EBps4800; break;
    case 7200: m_currSettings().iRate = EBps7200; break;
    case 9600: m_currSettings().iRate = EBps9600; break;
    case 19200: m_currSettings().iRate = EBps19200; break;
    case 38400: m_currSettings().iRate = EBps38400; break;
    case 57600: m_currSettings().iRate = EBps57600; break;
    case 115200: m_currSettings().iRate = EBps115200; break;
    case 230400: m_currSettings().iRate = EBps230400; break;
    case 460800: m_currSettings().iRate = EBps460800; break;
    case 576000: m_currSettings().iRate = EBps576000; break;
    case 1152000: m_currSettings().iRate = EBps1152000; break;
    case 4000000: m_currSettings().iRate = EBps4000000; break;
    case 921600: m_currSettings().iRate = EBps921600; break;
        //case 1843200:; // Only for  Symbian SR1
    default:
        setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }

    bool ret = updateCommConfig();
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

    switch (dataBits) {
    case SerialPort::Data5: m_currSettings().iDataBits = EData5; break;
    case SerialPort::Data6: m_currSettings().iDataBits = EData6; break;
    case SerialPort::Data7: m_currSettings().iDataBits = EData7; break;
    case SerialPort::Data8: m_currSettings().iDataBits = EData8; break;
    }

    bool ret = updateCommConfig();
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
    case SerialPort::NoParity: m_currSettings().iParity = EParityNone; break;
    case SerialPort::EvenParity: m_currSettings().iParity = EParityEven; break;
    case SerialPort::OddParity: m_currSettings().iParity = EParityOdd; break;
    case SerialPort::MarkParity: m_currSettings().iParity = EParityMark; break;
    case SerialPort::SpaceParity: m_currSettings().iParity = EParitySpace; break;
    }

    bool ret = updateCommConfig();
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
    case SerialPort::OneStop: m_currSettings().iStopBits = EStop1; break;
    case SerialPort::TwoStop: m_currSettings().iStopBits = EStop2; break;
    default:
        setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }

    bool ret = updateCommConfig();
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
    case SerialPort::NoFlowControl: m_currSettings().iHandshake = KConfigFailDSR; break;
    case SerialPort::HardwareControl: m_currSettings().iHandshake = KConfigObeyCTS | KConfigFreeRTS; break;
    case SerialPort::SoftwareControl: m_currSettings().iHandshake = KConfigObeyXoff | KConfigSendXoff; break;
    }

    bool ret = updateCommConfig();
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
    // Impl me
    return false;
}

bool SerialPortPrivate::setNativeDataErrorPolicy(SerialPort::DataErrorPolicy policy)
{
    Q_UNUSED(policy)
    return true;
}

bool SerialPortPrivate::nativeFlush()
{
    // Impl me
    return false;
}

void SerialPortPrivate::detectDefaultSettings()
{
    // Detect rate.
    switch (m_currSettings().iRate) {
    case EBps50: m_inRate = 50; break;
    case EBps75: m_inRate = 75; break;
    case EBps110: m_inRate = 110; break;
    case EBps134: m_inRate = 134; break;
    case EBps150: m_inRate = 150; break;
    case EBps300: m_inRate = 300; break;
    case EBps600: m_inRate = 600; break;
    case EBps1200: m_inRate = 1200; break;
    case EBps1800: m_inRate = 1800; break;
    case EBps2000: m_inRate = 2000; break;
    case EBps2400: m_inRate = 2400; break;
    case EBps3600: m_inRate = 3600; break;
    case EBps4800: m_inRate = 4800; break;
    case EBps7200: m_inRate = 7200; break;
    case EBps9600: m_inRate = 9600; break;
    case EBps19200: m_inRate = 19200; break;
    case EBps38400: m_inRate = 38400; break;
    case EBps57600: m_inRate = 57600; break;
    case EBps115200: m_inRate = 115200; break;
    case EBps230400: m_inRate = 230400; break;
    case EBps460800: m_inRate = 460800; break;
    case EBps576000: m_inRate = 576000; break;
    case EBps1152000: m_inRate = 1152000; break;
    case EBps4000000: m_inRate = 4000000; break;
    case EBps921600: m_inRate = 921600; break;
        //case EBps1843200: m_inRate = 1843200; break;
    default: m_inRate = SerialPort::UnknownRate;
    }
    m_outRate = m_inRate;

    // Detect databits.
    switch (m_currSettings().iDataBits) {
    case EData5: m_dataBits = SerialPort::Data5; break;
    case EData6: m_dataBits = SerialPort::Data6; break;
    case EData7: m_dataBits = SerialPort::Data7; break;
    case EData8: m_dataBits = SerialPort::Data8; break;
    default: m_dataBits = SerialPort::UnknownDataBits;
    }

    // Detect parity.
    switch (m_currSettings().iParity) {
    case EParityNone: m_parity = SerialPort::NoParity; break;
    case EParityEven: m_parity = SerialPort::EvenParity; break;
    case EParityOdd: m_parity = SerialPort::OddParity; break;
    case EParityMark: m_parity = SerialPort::MarkParity; break;
    case EParitySpace: m_parity = SerialPort::SpaceParity; break;
    default: m_parity = SerialPort::UnknownParity;
    }

    // Detect stopbits.
    switch (m_currSettings().iStopBits) {
    case EStop1: m_stopBits = SerialPort::OneStop;
    case EStop2: m_stopBits = SerialPort::TwoStop;
    default:  m_stopBits = SerialPort::UnknownStopBits;
    }

    // Detect flow control.
    if ((m_currSettings().iHandshake & (KConfigObeyXoff | KConfigSendXoff))
            == (KConfigObeyXoff | KConfigSendXoff))
        m_flow = SerialPort::SoftwareControl;
    else if ((m_currSettings().iHandshake & (KConfigObeyCTS | KConfigFreeRTS))
             == (KConfigObeyCTS | KConfigFreeRTS))
        m_flow = SerialPort::HardwareControl;
    else if (m_currSettings().iHandshake & KConfigFailDSR)
        m_flow = SerialPort::NoFlowControl;
    else
        m_flow = SerialPort::UnknownFlowControl;
}

// Used only in method SerialPortPrivate::open().
bool SerialPortPrivate::saveOldsettings()
{
    TInt r = m_descriptor.Config(m_oldSettings);
    if (r != KErrNone)
        return false;

    m_currSettings = m_oldSettings;
    m_oldSettingsIsSaved = true;
    return true;
}

// Used only in method SerialPortPrivate::close().
bool SerialPortPrivate::restoreOldsettings()
{
    TInt r = KErrNone;
    if (m_oldSettingsIsSaved) {
        m_oldSettingsIsSaved = false;
        r = m_descriptor.SetConfig(m_oldSettings);
    }
    return (r == KErrNone);
}


/* Private methods */


inline bool SerialPortPrivate::updateCommConfig()
{
    return (m_descriptor.SetConfig(m_currSettings) == KErrNone);
}

bool SerialPortPrivate::isRestrictedAreaSettings(SerialPort::DataBits dataBits,
                                                 SerialPort::StopBits stopBits) const
{
    // Impl me
    return (((dataBits == SerialPort::Data5) && (stopBits == SerialPort::TwoStop))
            || ((dataBits == SerialPort::Data6) && (stopBits == SerialPort::OneAndHalfStop))
            || ((dataBits == SerialPort::Data7) && (stopBits == SerialPort::OneAndHalfStop))
            || ((dataBits == SerialPort::Data8) && (stopBits == SerialPort::OneAndHalfStop)));
}

// Prepares other parameters of the structures port configuration.
// Used only in method SerialPortPrivate::open().
void SerialPortPrivate::prepareOtherOptions()
{
    // Impl me
}



