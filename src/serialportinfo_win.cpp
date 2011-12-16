/*
    License...
*/

#include "serialportinfo.h"
#include "serialportinfo_p.h"

#include <qt_windows.h>
#include <objbase.h>
#include <initguid.h>
#if !defined (Q_OS_WINCE)
#  include <setupapi.h>
#endif

#include <QtCore/qvariant.h>
#include <QtCore/qstringlist.h>

QT_USE_NAMESPACE

static const GUID guidArray[] =
{
#if !defined (Q_OS_WINCE)
    /* Windows Ports Class GUID */
    { 0x4D36E978, 0xE325, 0x11CE, { 0xBF, 0xC1, 0x08, 0x00, 0x2B, 0xE1, 0x03, 0x18 } },
    /* Virtual Ports Class GUID (i.e. com0com and etc) */
    { 0xDF799E12, 0x3C56, 0x421B, { 0xB2, 0x98, 0xB6, 0xD3, 0x64, 0x2B, 0xC8, 0x78 } },
    /* Windows Modems Class GUID */
    { 0x4D36E96D, 0xE325, 0x11CE, { 0xBF, 0xC1, 0x08, 0x00, 0x2B, 0xE1, 0x03, 0x18 } },
    /* GUID for old version of Eltima Virtual Serial Port Driver v4 */
    { 0xCC0EF009, 0xB820, 0x42F4, { 0x95, 0xA9, 0x9B, 0xFA, 0x6A, 0x5A, 0xB7, 0xAB } }
#else
    { 0xCC5195AC, 0xBA49, 0x48A0, { 0xBE, 0x17, 0xDF, 0x6D, 0x1B, 0x01, 0x73, 0xDD } }
#endif
};

#if !defined (Q_OS_WINCE)

static QVariant getDeviceRegistryProperty(HDEVINFO deviceInfoSet,
                                          PSP_DEVINFO_DATA deviceInfoData,
                                          DWORD property)
{
    DWORD dataType = 0;
    DWORD dataSize = 0;
    QVariant v;

    ::SetupDiGetDeviceRegistryProperty(deviceInfoSet, deviceInfoData,
                                       property, &dataType, 0, 0, &dataSize);

    QByteArray data(dataSize, 0);

    if (::SetupDiGetDeviceRegistryProperty(deviceInfoSet, deviceInfoData,
                                           property, 0,
                                           reinterpret_cast<unsigned char*>(data.data()),
                                           dataSize, 0)) {

        switch (dataType) {

        case REG_EXPAND_SZ:
        case REG_SZ: {
            QString s;
            if (dataSize) {
                s = QString::fromWCharArray(((const wchar_t *)data.constData()));
            }
            v = QVariant(s);
            break;
        }

        case REG_MULTI_SZ: {
            QStringList l;
            if (dataSize) {
                int i = 0;
                for (;;) {
                    QString s = QString::fromWCharArray((const wchar_t *)data.constData() + i);
                    i += s.length() + 1;

                    if (s.isEmpty())
                        break;
                    l.append(s);
                }
            }
            v = QVariant(l);
            break;
        }

        case REG_NONE:
        case REG_BINARY: {
            QString s;
            if (dataSize) {
                s = QString::fromWCharArray((const wchar_t *)data.constData(), data.size() / 2);
            }
            v = QVariant(s);
            break;
        }

        case REG_DWORD_BIG_ENDIAN:
        case REG_DWORD: {
            Q_ASSERT(data.size() == sizeof(int));
            int i = 0;
            ::memcpy((void *)(&i), data.constData(), sizeof(int));
            v = i;
            break;
        }

        default:
            v = QVariant();
        }

    }

    return v;
}

static QString getNativeName(HDEVINFO deviceInfoSet,
                             PSP_DEVINFO_DATA deviceInfoData) {

    HKEY key = ::SetupDiOpenDevRegKey(deviceInfoSet, deviceInfoData, DICS_FLAG_GLOBAL,
                                      0, DIREG_DEV, KEY_READ);

    QString result;

    if (key == INVALID_HANDLE_VALUE)
        return result;

    DWORD index = 0;
    QByteArray bufKeyName(16384, 0);
    QByteArray bufKeyVal(16384, 0);

    for (;;) {
        DWORD lenKeyName = bufKeyName.size();
        DWORD lenKeyValue = bufKeyVal.size();
        DWORD keyType = 0;
        LONG ret = ::RegEnumValue(key,
                                  index++,
                                  reinterpret_cast<wchar_t *>(bufKeyName.data()), &lenKeyName,
                                  0,
                                  &keyType,
                                  reinterpret_cast<unsigned char *>(bufKeyVal.data()), &lenKeyValue);

        if (ret == ERROR_SUCCESS) {
            if (keyType == REG_SZ) {

                QString itemName = QString::fromUtf16(reinterpret_cast<ushort *>(bufKeyName.data()), lenKeyName);
                QString itemValue = QString::fromUtf16(((const ushort *)bufKeyVal.constData()));

                if (itemName.contains("PortName")) {
                    result = itemValue;
                    break;
                }
            }
        }
        else
            break;
    }

    ::RegCloseKey(key);

    return result;
}

#else

const static QString valueName = "FriendlyName";
static QString findDescription(HKEY parentKeyHandle, const QString &subKey)
{
    QString result;
    HKEY hSubKey = 0;
    LONG res = ::RegOpenKeyEx(parentKeyHandle, reinterpret_cast<const wchar_t *>(subKey.utf16()),
                              0, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, &hSubKey);

    if (res == ERROR_SUCCESS) {

        DWORD dataType = 0;
        DWORD dataSize = 0;
        res = ::RegQueryValueEx(hSubKey, reinterpret_cast<const wchar_t *>(valueName.utf16()),
                                0, &dataType, 0, &dataSize);

        if (res == ERROR_SUCCESS) {
            QByteArray data(dataSize, 0);
            res = ::RegQueryValueEx(hSubKey, reinterpret_cast<const wchar_t *>(valueName.utf16()),
                                    0, 0,
                                    reinterpret_cast<unsigned char *>(data.data()),
                                    &dataSize);

            if (res == ERROR_SUCCESS) {
                switch (dataType) {
                case REG_EXPAND_SZ:
                case REG_SZ:
                    if (dataSize)
                        result = QString::fromWCharArray(((const wchar_t *)data.constData()));
                    break;
                default:;
                }
            }
        } else {
            DWORD index = 0;
            dataSize = 255; // Max. key length (see MSDN).
            QByteArray data(dataSize, 0);
            while (::RegEnumKeyEx(hSubKey, index++,
                                  reinterpret_cast<wchar_t *>(data.data()), &dataSize,
                                  0, 0, 0, 0) == ERROR_SUCCESS) {

                result = findDescription(hSubKey,
                                         QString::fromUtf16(reinterpret_cast<ushort *>(data.data()), dataSize));
                if (!result.isEmpty())
                    break;
            }
        }
        ::RegCloseKey(hSubKey);
    }
    return result;
}

#endif

/* Public methods */


QList<SerialPortInfo> SerialPortInfo::availablePorts()
{
    QList<SerialPortInfo> ports;
    static int guidCount = sizeof(guidArray)/sizeof(GUID);

#if !defined (Q_OS_WINCE)
    for (int i = 0; i < guidCount; ++i) {

        HDEVINFO deviceInfoSet = ::SetupDiGetClassDevs(&guidArray[i], 0, 0, DIGCF_PRESENT);

        if (deviceInfoSet == INVALID_HANDLE_VALUE)
            return ports;

        SP_DEVINFO_DATA deviceInfoData;
        int size = sizeof(SP_DEVINFO_DATA);
        ::memset(&deviceInfoData, 0, size);
        deviceInfoData.cbSize = size;

        DWORD index = 0;
        while (::SetupDiEnumDeviceInfo(deviceInfoSet, index++, &deviceInfoData)) {

            SerialPortInfo info;
            QVariant v = getNativeName(deviceInfoSet, &deviceInfoData);
            QString s = v.toString();

            if (!(s.isEmpty() || s.contains("LPT"))) {

                info.d_ptr->portName = s;
                info.d_ptr->device = "\\\\.\\" + s;

                v = getDeviceRegistryProperty(deviceInfoSet, &deviceInfoData, SPDRP_DEVICEDESC);
                info.d_ptr->description = v.toString();

                v = getDeviceRegistryProperty(deviceInfoSet, &deviceInfoData, SPDRP_MFG);
                info.d_ptr->manufacturer = v.toString();

                ports.append(info);
            }
        }

        ::SetupDiDestroyDeviceInfoList(deviceInfoSet);
    }
#else
    //for (int i = 0; i < guidCount; ++i) {
    DEVMGR_DEVICE_INFORMATION di;
    di.dwSize = sizeof(DEVMGR_DEVICE_INFORMATION);
    HANDLE hSearch = ::FindFirstDevice(DeviceSearchByLegacyName/*DeviceSearchByGuid*/, L"COM*"/*&guidArray[i]*/, &di);
    if (hSearch != INVALID_HANDLE_VALUE) {
        do {
            SerialPortInfo info;
            info.d_ptr->device = QString::fromWCharArray(((const wchar_t *)di.szLegacyName));
            info.d_ptr->portName = info.d_ptr->device;
            info.d_ptr->portName.remove(':');
            info.d_ptr->description = findDescription(HKEY_LOCAL_MACHINE,
                                                      QString::fromWCharArray(((const wchar_t *)di.szDeviceKey)));
            if (info.d_ptr->description.isEmpty())
                info.d_ptr->description = QString(QObject::tr("Unknown."));
            info.d_ptr->manufacturer = QString(QObject::tr("Unknown."));

            ports.append(info);

        } while (::FindNextDevice(hSearch, &di));
        ::FindClose(hSearch);
    }
    //}
#endif
    return ports;
}


QList<qint32> SerialPortInfo::standardRates() const
{
    QList<qint32> rates;

    QString location = systemLocation();
    QByteArray nativeFilePath = QByteArray((const char *)location.utf16(), location.size() * 2 + 1);

    HANDLE descriptor = ::CreateFile((const wchar_t*)nativeFilePath.constData(),
                                     GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);

    if (descriptor != INVALID_HANDLE_VALUE) {
        COMMPROP prop;
        if ((::GetCommProperties(descriptor, &prop) != 0) && prop.dwSettableBaud) {
            if (prop.dwSettableBaud & BAUD_075)
                rates.append(75);
            if (prop.dwSettableBaud & BAUD_110)
                rates.append(110);
            if (prop.dwSettableBaud & BAUD_150)
                rates.append(150);
            if (prop.dwSettableBaud & BAUD_300)
                rates.append(300);
            if (prop.dwSettableBaud & BAUD_600)
                rates.append(600);
            if (prop.dwSettableBaud & BAUD_1200)
                rates.append(1200);
            if (prop.dwSettableBaud & BAUD_1800)
                rates.append(1800);
            if (prop.dwSettableBaud & BAUD_2400)
                rates.append(2400);
            if (prop.dwSettableBaud & BAUD_4800)
                rates.append(4800);
            if (prop.dwSettableBaud & BAUD_7200)
                rates.append(7200);
            if (prop.dwSettableBaud & BAUD_9600)
                rates.append(9600);
            if (prop.dwSettableBaud & BAUD_14400)
                rates.append(14400);
            if (prop.dwSettableBaud & BAUD_19200)
                rates.append(19200);
            if (prop.dwSettableBaud & BAUD_38400)
                rates.append(38400);
            if (prop.dwSettableBaud & BAUD_56K)
                rates.append(56000);
            if (prop.dwSettableBaud & BAUD_57600)
                rates.append(57600);
            if (prop.dwSettableBaud & BAUD_115200)
                rates.append(115200);
            if (prop.dwSettableBaud & BAUD_128K)
                rates.append(128000);
        }
        ::CloseHandle(descriptor);
    }

    return rates;
}

bool SerialPortInfo::isBusy() const
{
    QString location = systemLocation();
    QByteArray nativeFilePath = QByteArray((const char *)location.utf16(), location.size() * 2 + 1);

    HANDLE descriptor = ::CreateFile((const wchar_t*)nativeFilePath.constData(),
                                     GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);

    if (descriptor == INVALID_HANDLE_VALUE) {
        if (::GetLastError() == ERROR_ACCESS_DENIED)
            return true;
    }
    else
        ::CloseHandle(descriptor);
    return false;
}

bool SerialPortInfo::isValid() const
{
    QString location = systemLocation();
    QByteArray nativeFilePath = QByteArray((const char *)location.utf16(), location.size() * 2 + 1);

    HANDLE descriptor = ::CreateFile((const wchar_t*)nativeFilePath.constData(),
                                     GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);

    if (descriptor == INVALID_HANDLE_VALUE) {
        if (::GetLastError() != ERROR_ACCESS_DENIED)
            return false;
    }
    else
        ::CloseHandle(descriptor);
    return true;
}
