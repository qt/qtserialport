/*
    License...
*/

#include "serialportinfo.h"
#include "serialportinfo_p.h"

#include <e32base.h>
//#include <e32test.h>
#include <c32comm.h>
#include <f32file.h>

#include <QtCore/qobject.h>


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

QT_USE_NAMESPACE

/* Public methods */

QList<SerialPortInfo> SerialPortInfo::availablePorts()
{
    QList<SerialPortInfo> ports;

    if (!loadDevices())
        return ports;
    
    RCommServ server;
    TInt r = server.Connect();
    if (r != KErrNone)
        return ports; //User::LeaveIfError(r);

    TSerialInfo nativeInfo; // Native Symbian OS port info class.
    QString s("%1::%2");

    // FIXME: Get info about RS232 ports.
    r = server.LoadCommModule(KRS232ModuleName);
    //User::LeaveIfError(r);
    if (r == KErrNone) {
        r = server.GetPortInfo(KRS232ModuleName, nativeInfo);
        if (r == KErrNone) {
            //
            for (int i = nativeInfo.iLowUnit; i < nativeInfo.iHighUnit + 1; ++i) {

                SerialPortInfo info; // My (desired) info class.

                info.d_ptr->device = s
                        .arg(QString::fromUtf16(nativeInfo.iName.Ptr(), nativeInfo.iName.Length()))
                        .arg(i);
                info.d_ptr->portName = info.d_ptr->device;
                info.d_ptr->description =
                        QString::fromUtf16(nativeInfo.iDescription.Ptr(), nativeInfo.iDescription.Length());
                info.d_ptr->manufacturer = QString(QObject::tr("Unknown."));
                ports.append(info);
            }
        }
    }

    // FIXME: Get info about Bluetooth ports.
    r = server.LoadCommModule(KBluetoothModuleName);
    //User::LeaveIfError(r);
    if (r == KErrNone) {
        r = server.GetPortInfo(KBluetoothModuleName, nativeInfo);
        if (r == KErrNone) {
            //
            for (int i = nativeInfo.iLowUnit; i < nativeInfo.iHighUnit + 1; ++i) {

                SerialPortInfo info; // My (desired) info class.

                info.d_ptr->device = s
                        .arg(QString::fromUtf16(nativeInfo.iName.Ptr(), nativeInfo.iName.Length()))
                        .arg(i);
                info.d_ptr->portName = info.d_ptr->device;
                info.d_ptr->description =
                        QString::fromUtf16(nativeInfo.iDescription.Ptr(), nativeInfo.iDescription.Length());
                info.d_ptr->manufacturer = QString(QObject::tr("Unknown."));
                ports.append(info);
            }
        }
    }
    
    // FIXME: Get info about InfraRed ports.
    r = server.LoadCommModule(KInfraRedModuleName);
    //User::LeaveIfError(r);
    if (r == KErrNone) {
        r = server.GetPortInfo(KInfraRedModuleName, nativeInfo);
        if (r == KErrNone) {
            //
            for (int i = nativeInfo.iLowUnit; i < nativeInfo.iHighUnit + 1; ++i) {

                SerialPortInfo info; // My (desired) info class.

                info.d_ptr->device = s
                        .arg(QString::fromUtf16(nativeInfo.iName.Ptr(), nativeInfo.iName.Length()))
                        .arg(i);
                info.d_ptr->portName = info.d_ptr->device;
                info.d_ptr->description =
                        QString::fromUtf16(nativeInfo.iDescription.Ptr(), nativeInfo.iDescription.Length());
                info.d_ptr->manufacturer = QString(QObject::tr("Unknown."));
                ports.append(info);
            }
        }
    }

    return ports;
}

QList<qint32> SerialPortInfo::standardRates() const
{
    QList<qint32> rates;
    
    // See enum TBps in SDK:
    // - epoc32/include/platform/d32comm.h for Symbian^3
    // - epoc32/include/platform/d32public.h for Symbian SR1
    rates.append(50);
    rates.append(75);
    rates.append(110);
    rates.append(134);
    rates.append(150);
    rates.append(300);
    rates.append(600);
    rates.append(1200);
    rates.append(1800);
    rates.append(2000);
    rates.append(2400);
    rates.append(3600);
    rates.append(4800);
    rates.append(7200);
    rates.append(9600);
    rates.append(19200);
    rates.append(38400);
    rates.append(57600);
    rates.append(115200);
    rates.append(230400);
    rates.append(460800);
    rates.append(576000);
    rates.append(1152000);
    rates.append(4000000);
    rates.append(921600);
    rates.append(1843200); // Only for  Symbian SR1
    return rates;
}

bool SerialPortInfo::isBusy() const
{
    if (!loadDevices())
        return false;

    RCommServ server;
    TInt r = server.Connect();
    if (r != KErrNone)
        return false;

    RComm port;
    TPtrC portName(static_cast<const TUint16*>(systemLocation().utf16()), systemLocation().length());
    r = port.Open(server, portName, ECommExclusive);
    if (r == KErrNone)
        port.Close();
    return (r == KErrLocked);
}

bool SerialPortInfo::isValid() const
{
    if (!loadDevices())
        return false;

    RCommServ server;
    TInt r = server.Connect();
    if (r != KErrNone)
        return false;

    RComm port;
    TPtrC portName(static_cast<const TUint16*>(systemLocation().utf16()), systemLocation().length());
    r = port.Open(server, portName, ECommExclusive);
    if (r == KErrNone)
        port.Close();
    return (r == KErrNone ) || (r == KErrLocked);
}
