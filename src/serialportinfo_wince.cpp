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

#include <QtCore/qvariant.h>
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE_SERIALPORT

static const GUID guidsArray[] =
{
    { 0xCC5195AC, 0xBA49, 0x48A0, { 0xBE, 0x17, 0xDF, 0x6D, 0x1B, 0x01, 0x73, 0xDD } }
};

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
                        result = QString::fromWCharArray(reinterpret_cast<const wchar_t *>(data.constData()));
                    break;
                default:
                    break;
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

QList<SerialPortInfo> SerialPortInfo::availablePorts()
{
    QList<SerialPortInfo> ports;

    DEVMGR_DEVICE_INFORMATION di;
    di.dwSize = sizeof(di);
    const HANDLE hSearch = ::FindFirstDevice(DeviceSearchByLegacyName,
                                             L"COM*",
                                             &di);
    if (hSearch != INVALID_HANDLE_VALUE) {
        do {
            SerialPortInfo info;
            info.d_ptr->device = QString::fromWCharArray(di.szLegacyName);
            info.d_ptr->portName = SerialPortPrivate::portNameFromSystemLocation(info.d_ptr->device);
            info.d_ptr->description = findDescription(HKEY_LOCAL_MACHINE,
                                                      QString::fromWCharArray(di.szDeviceKey));

            // Get manufacturer, vendor identifier, product identifier are not
            // possible.

            ports.append(info);

        } while (::FindNextDevice(hSearch, &di));
        ::FindClose(hSearch);
    }

    return ports;
}


QList<qint32> SerialPortInfo::standardRates()
{
    return SerialPortPrivate::standardRates();
}

bool SerialPortInfo::isBusy() const
{
    const HANDLE descriptor = ::CreateFile(reinterpret_cast<const wchar_t*>(systemLocation().utf16()),
                                           GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);

    if (descriptor == INVALID_HANDLE_VALUE) {
        if (::GetLastError() == ERROR_ACCESS_DENIED)
            return true;
    } else {
        ::CloseHandle(descriptor);
    }
    return false;
}

bool SerialPortInfo::isValid() const
{
    const HANDLE descriptor = ::CreateFile(reinterpret_cast<const wchar_t*>(systemLocation().utf16()),
                                           GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);

    if (descriptor == INVALID_HANDLE_VALUE) {
        if (::GetLastError() != ERROR_ACCESS_DENIED)
            return false;
    } else {
        ::CloseHandle(descriptor);
    }
    return true;
}

QT_END_NAMESPACE_SERIALPORT
