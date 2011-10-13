/*
    License...
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

SymbianSerialPortEngine::SymbianSerialPortEngine(SerialPortPrivate *parent)
{
    Q_ASSERT(parent);
    // Impl me
    m_parent = parent;
    m_oldSettingsIsSaved = false;
}

SymbianSerialPortEngine::~SymbianSerialPortEngine()
{

}

bool SymbianSerialPortEngine::open(const QString &location, QIODevice::OpenMode mode)
{
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

    if (saveOldsettings()) {
        detectDefaultSettings();
        return true;
    }
    m_parent->setError(SerialPort::ConfiguringError);
    return false;
}

void SymbianSerialPortEngine::close(const QString &location)
{
    Q_UNUSED(location);
    restoreOldsettings();
    m_descriptor.Close();
}

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

bool SymbianSerialPortEngine::setDtr(bool set)
{
    if (set)
        m_descriptor.SetSignalsToMark(KSignalDTR);
    else
        m_descriptor.SetSignalsToSpace(KSignalDTR);
    return true;
}

bool SymbianSerialPortEngine::setRts(bool set)
{
    if (set)
        m_descriptor.SetSignalsToMark(KSignalRTS);
    else
        m_descriptor.SetSignalsToSpace(KSignalRTS);
    return true;
}

bool SymbianSerialPortEngine::flush()
{
    // Impl me
    return false;
}

bool SymbianSerialPortEngine::reset()
{
    m_descriptor.ResetBuffers(KCommResetRx | KCommResetTx);
    return true;
}

bool SymbianSerialPortEngine::sendBreak(int duration)
{
    TRequestStatus status;
    m_descriptor.Break(status, TTimeIntervalMicroSeconds32(duration * 1000));
    return false;
}

bool SymbianSerialPortEngine::setBreak(bool set)
{
    // Impl me
    return false;
}

qint64 SymbianSerialPortEngine::bytesAvailable() const
{
    return qint64(m_descriptor.QueryReceiveBuffer());
}

qint64 SymbianSerialPortEngine::bytesToWrite() const
{
    return 0;
}

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

// FIXME: I'm not sure in implementation this method.
// Someone needs to check and correct.
bool SymbianSerialPortEngine::select(int timeout,
                                     bool checkRead, bool checkWrite,
                                     bool *selectForRead, bool *selectForWrite)
{
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

static const QString defaultPathPostfix = ":";

QString SymbianSerialPortEngine::toSystemLocation(const QString &port) const
{
    // Port name is equval to port location.
    return port;
}

QString SymbianSerialPortEngine::fromSystemLocation(const QString &location) const
{
    // Port name is equval to port location.
    return location;
}

bool SymbianSerialPortEngine::setRate(qint32 rate, SerialPort::Directions dir)
{
    if ((rate == SerialPort::UnknownRate) || (dir != SerialPort::AllDirections)) {
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

    bool ret = updateCommConfig();
    if (!ret)
        m_parent->setError(SerialPort::ConfiguringError);
    return ret;
}

bool SymbianSerialPortEngine::setDataBits(SerialPort::DataBits dataBits)
{
    if ((dataBits == SerialPort::UnknownDataBits)
            || isRestrictedAreaSettings(dataBits, m_parent->m_stopBits)) {

        m_parent->setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }

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
    }

    bool ret = updateCommConfig();
    if (!ret)
        m_parent->setError(SerialPort::ConfiguringError);
    return ret;
}

bool SymbianSerialPortEngine::setParity(SerialPort::Parity parity)
{
    if (parity == SerialPort::UnknownParity) {
        m_parent->setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }

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
    }

    bool ret = updateCommConfig();
    if (!ret)
        m_parent->setError(SerialPort::ConfiguringError);
    return ret;
}

bool SymbianSerialPortEngine::setStopBits(SerialPort::StopBits stopBits)
{
    if ((stopBits == SerialPort::UnknownStopBits)
            || isRestrictedAreaSettings(m_parent->m_dataBits, stopBits)) {

        m_parent->setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }

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

    bool ret = updateCommConfig();
    if (!ret)
        m_parent->setError(SerialPort::ConfiguringError);
    return ret;
}

bool SymbianSerialPortEngine::setFlowControl(SerialPort::FlowControl flow)
{
    if (flow == SerialPort::UnknownFlowControl) {
        m_parent->setError(SerialPort::UnsupportedPortOperationError);
        return false;
    }

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
    }

    bool ret = updateCommConfig();
    if (!ret)
        m_parent->setError(SerialPort::ConfiguringError);
    return ret;
}

bool SymbianSerialPortEngine::setDataErrorPolicy(SerialPort::DataErrorPolicy policy)
{
    Q_UNUSED(policy)
    // Impl me
    return true;
}

bool SymbianSerialPortEngine::isReadNotificationEnabled() const
{
    // Impl me
    return false;
}

void SymbianSerialPortEngine::setReadNotificationEnabled(bool enable)
{
    Q_UNUSED(enable)
    // Impl me
}

bool SymbianSerialPortEngine::isWriteNotificationEnabled() const
{
    // Impl me
    return false;
}

void SymbianSerialPortEngine::setWriteNotificationEnabled(bool enable)
{
    Q_UNUSED(enable)
    // Impl me
}

bool SymbianSerialPortEngine::processIOErrors()
{
    // Impl me
    return false;
}

void SymbianSerialPortEngine::lockNotification(NotificationLockerType type, bool uselocker)
{
    Q_UNUSED(type);
    Q_UNUSED(uselocker);
    // For Symbian is not used! Used only for WinCE!
}

void SymbianSerialPortEngine::unlockNotification(NotificationLockerType type)
{
    Q_UNUSED(type);
    // For Symbian is not used! Used only for WinCE!
}

/* Protected methods */

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

// Used only in method SymbianSerialPortEngine::open().
bool SymbianSerialPortEngine::saveOldsettings()
{
    TInt r = m_descriptor.Config(m_oldSettings);
    if (r != KErrNone)
        return false;

    m_currSettings = m_oldSettings;
    m_oldSettingsIsSaved = true;
    return true;
}

// Used only in method SymbianSerialPortEngine::close().
bool SymbianSerialPortEngine::restoreOldsettings()
{
    TInt r = KErrNone;
    if (m_oldSettingsIsSaved) {
        m_oldSettingsIsSaved = false;
        r = m_descriptor.SetConfig(m_oldSettings);
    }
    return (r == KErrNone);
}

// Prepares other parameters of the structures port configuration.
// Used only in method SymbianSerialPortEngine::open().
void SymbianSerialPortEngine::prepareOtherOptions()
{
    // Impl me
}

/* Private methods */

inline bool SymbianSerialPortEngine::updateCommConfig()
{
    return (m_descriptor.SetConfig(m_currSettings) == KErrNone);
}

bool SymbianSerialPortEngine::isRestrictedAreaSettings(SerialPort::DataBits dataBits,
                                                       SerialPort::StopBits stopBits) const
{
    // Impl me
    return (((dataBits == SerialPort::Data5) && (stopBits == SerialPort::TwoStop))
            || ((dataBits == SerialPort::Data6) && (stopBits == SerialPort::OneAndHalfStop))
            || ((dataBits == SerialPort::Data7) && (stopBits == SerialPort::OneAndHalfStop))
            || ((dataBits == SerialPort::Data8) && (stopBits == SerialPort::OneAndHalfStop)));
}
// From <serialportengine_p.h>
SerialPortEngine *SerialPortEngine::create(SerialPortPrivate *parent)
{
    return new SymbianSerialPortEngine(parent);
}




