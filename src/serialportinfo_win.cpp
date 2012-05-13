/****************************************************************************
**
** Copyright (C) 2011-2012 Denis Shienkov <scapig2@yandex.ru>
** Copyright (C) 2011 Sergey Belyashov <Sergey.Belyashov@gmail.com>
** Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
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

#include "serialportinfo.h"
#include "serialportinfo_p.h"
#include "serialportengine_win_p.h"

#include <qt_windows.h>
#include <objbase.h>
#include <initguid.h>
#if !defined (Q_OS_WINCE)
#  include <setupapi.h>
#endif

#include <QtCore/qvariant.h>
#include <QtCore/qstringlist.h>


static const GUID guidsArray[] =
{
#if !defined (Q_OS_WINCE)
    // Windows Ports Class GUID
    { 0x4D36E978, 0xE325, 0x11CE, { 0xBF, 0xC1, 0x08, 0x00, 0x2B, 0xE1, 0x03, 0x18 } },
    // Virtual Ports Class GUID (i.e. com0com and etc)
    { 0xDF799E12, 0x3C56, 0x421B, { 0xB2, 0x98, 0xB6, 0xD3, 0x64, 0x2B, 0xC8, 0x78 } },
    // Windows Modems Class GUID
    { 0x4D36E96D, 0xE325, 0x11CE, { 0xBF, 0xC1, 0x08, 0x00, 0x2B, 0xE1, 0x03, 0x18 } },
    // Eltima Virtual Serial Port Driver v4 GUID
    { 0xCC0EF009, 0xB820, 0x42F4, { 0x95, 0xA9, 0x9B, 0xFA, 0x6A, 0x5A, 0xB7, 0xAB } },
    // Advanced Virtual COM Port GUID
    { 0x9341CD95, 0x4371, 0x4A37, { 0xA5, 0xAF, 0xFD, 0xB0, 0xA9, 0xD1, 0x96, 0x31 } },
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

                if (itemName.contains(QLatin1String("PortName"))) {
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

// Regular expression pattern for extraction a VID/PID from Hardware ID
const static QLatin1String hardwareIdPattern("vid_(\\w+)&pid_(\\w+)");

// Command for extraction desired VID/PID from Hardware ID.
enum ExtractCommand { CMD_EXTRACT_VID = 1, CMD_EXTRACT_PID = 2 };

// Extract desired part Id type from Hardware ID.
static QString parceHardwareId(ExtractCommand cmd, const QStringList &hardwareId)
{
    QRegExp rx(hardwareIdPattern);
    rx.setCaseSensitivity(Qt::CaseInsensitive);
    rx.indexIn(hardwareId.at(0));
    return rx.cap(cmd);
}

#else

const static QString valueName(QLatin1String("FriendlyName"));
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

QT_BEGIN_NAMESPACE_SERIALPORT

/* Public methods */


QList<SerialPortInfo> SerialPortInfo::availablePorts()
{
    QList<SerialPortInfo> ports;
    static int guidCount = sizeof(guidsArray)/sizeof(GUID);

#if !defined (Q_OS_WINCE)
    for (int i = 0; i < guidCount; ++i) {

        HDEVINFO deviceInfoSet = ::SetupDiGetClassDevs(&guidsArray[i], 0, 0, DIGCF_PRESENT);

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

            if (!(s.isEmpty() || s.contains(QLatin1String("LPT")))) {

                info.d_ptr->portName = s;
                info.d_ptr->device = QLatin1String("\\\\.\\") + s;

                v = getDeviceRegistryProperty(deviceInfoSet, &deviceInfoData, SPDRP_DEVICEDESC);
                info.d_ptr->description = v.toString();

                v = getDeviceRegistryProperty(deviceInfoSet, &deviceInfoData, SPDRP_MFG);
                info.d_ptr->manufacturer = v.toString();

                v = getDeviceRegistryProperty(deviceInfoSet, &deviceInfoData, SPDRP_HARDWAREID);
                info.d_ptr->vendorIdentifier = parceHardwareId(CMD_EXTRACT_VID, v.toStringList());
                info.d_ptr->productIdentifier = parceHardwareId(CMD_EXTRACT_PID, v.toStringList());

                ports.append(info);
            }
        }

        ::SetupDiDestroyDeviceInfoList(deviceInfoSet);
    }
#else
    //for (int i = 0; i < guidCount; ++i) {
    DEVMGR_DEVICE_INFORMATION di;
    di.dwSize = sizeof(DEVMGR_DEVICE_INFORMATION);
    HANDLE hSearch = ::FindFirstDevice(DeviceSearchByLegacyName/*DeviceSearchByGuid*/, L"COM*"/*&guidsArray[i]*/, &di);
    if (hSearch != INVALID_HANDLE_VALUE) {
        do {
            SerialPortInfo info;
            info.d_ptr->device = QString::fromWCharArray(((const wchar_t *)di.szLegacyName));
            info.d_ptr->portName = info.d_ptr->device;
            info.d_ptr->portName.remove(':');
            info.d_ptr->description = findDescription(HKEY_LOCAL_MACHINE,
                                                      QString::fromWCharArray(((const wchar_t *)di.szDeviceKey)));

            // Get manufacturer, vendor identifier, product identifier are not
            // possible.

            ports.append(info);

        } while (::FindNextDevice(hSearch, &di));
        ::FindClose(hSearch);
    }
    //}
#endif
    return ports;
}


QList<qint32> SerialPortInfo::standardRates()
{
    return WinSerialPortEngine::standardRates();
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

QT_END_NAMESPACE_SERIALPORT
