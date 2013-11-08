/****************************************************************************
**
** Copyright (C) 2011-2012 Denis Shienkov <denis.shienkov@gmail.com>
** Copyright (C) 2011 Sergey Belyashov <Sergey.Belyashov@gmail.com>
** Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtSerialPort module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qserialportinfo.h"
#include "qserialportinfo_p.h"
#include "qserialport_win_p.h"

#ifndef Q_OS_WINCE
#include <initguid.h>
#include <setupapi.h>
#endif

#include <QtCore/qvariant.h>
#include <QtCore/qstringlist.h>
#include <QtCore/quuid.h>
#include <QtCore/qpair.h>

QT_BEGIN_NAMESPACE

#ifndef Q_OS_WINCE

typedef QPair<QUuid, DWORD> GuidFlagsPair;

static inline const QList<GuidFlagsPair>& guidFlagsPairs()
{
    static const QList<GuidFlagsPair> guidFlagsPairList = QList<GuidFlagsPair>()
               // Standard Setup Ports Class GUID
            << qMakePair(QUuid(0x4D36E978, 0xE325, 0x11CE, 0xBF, 0xC1, 0x08, 0x00, 0x2B, 0xE1, 0x03, 0x18), DWORD(DIGCF_PRESENT))
               // Standard Setup Modems Class GUID
            << qMakePair(QUuid(0x4D36E96D, 0xE325, 0x11CE, 0xBF, 0xC1, 0x08, 0x00, 0x2B, 0xE1, 0x03, 0x18), DWORD(DIGCF_PRESENT))
               // Standard Serial Port Device Interface Class GUID
            << qMakePair(QUuid(0x86E0D1E0, 0x8089, 0x11D0, 0x9C, 0xE4, 0x08, 0x00, 0x3E, 0x30, 0x1F, 0x73), DWORD(DIGCF_PRESENT | DIGCF_DEVICEINTERFACE))
               // Standard Modem Device Interface Class GUID
            << qMakePair(QUuid(0x2C7089AA, 0x2E0E, 0x11D1, 0xB1, 0x14, 0x00, 0xC0, 0x4F, 0xC2, 0xAA, 0xE4), DWORD(DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));
    return guidFlagsPairList;
}

static QVariant deviceRegistryProperty(HDEVINFO deviceInfoSet,
                                          PSP_DEVINFO_DATA deviceInfoData,
                                          DWORD property)
{
    DWORD dataType = 0;
    DWORD dataSize = 0;
    ::SetupDiGetDeviceRegistryProperty(deviceInfoSet, deviceInfoData,
                                       property, &dataType, NULL, 0, &dataSize);
    QByteArray data(dataSize, 0);
    if (!::SetupDiGetDeviceRegistryProperty(deviceInfoSet, deviceInfoData, property, NULL,
                                            reinterpret_cast<unsigned char*>(data.data()),
                                            dataSize, NULL)
            || !dataSize) {
        return QVariant();
    }

    switch (dataType) {

    case REG_EXPAND_SZ:
    case REG_SZ: {
        return QVariant(QString::fromWCharArray(reinterpret_cast<const wchar_t *>(data.constData())));
    }

    case REG_MULTI_SZ: {
        QStringList list;
        int i = 0;
        forever {
            QString s = QString::fromWCharArray(reinterpret_cast<const wchar_t *>(data.constData()) + i);
            i += s.length() + 1;
            if (s.isEmpty())
                break;
            list.append(s);
        }
        return QVariant(list);
    }

    default:
        break;
    }

    return QVariant();
}

static QString deviceInstanceIdentifier(HDEVINFO deviceInfoSet,
                                        PSP_DEVINFO_DATA deviceInfoData)
{
    DWORD requiredSize = 0;
    if (::SetupDiGetDeviceInstanceId(deviceInfoSet, deviceInfoData, NULL, 0, &requiredSize))
        return QString();

    QByteArray data(requiredSize * sizeof(wchar_t), 0);
    if (!::SetupDiGetDeviceInstanceId(deviceInfoSet, deviceInfoData,
                                      reinterpret_cast<wchar_t *>(data.data()), data.size(), NULL)) {
        // TODO: error handling with GetLastError
        return QString();
    }

    return QString::fromWCharArray(reinterpret_cast<const wchar_t *>(data.constData()));
}

static QString devicePortName(HDEVINFO deviceInfoSet, PSP_DEVINFO_DATA deviceInfoData)
{
    static const wchar_t portKeyName[] = L"PortName";

    const HKEY key = ::SetupDiOpenDevRegKey(deviceInfoSet, deviceInfoData, DICS_FLAG_GLOBAL,
                                            0, DIREG_DEV, KEY_READ);
    if (key == INVALID_HANDLE_VALUE)
        return QString();

    DWORD dataSize;
    if (::RegQueryValueEx(key, portKeyName, NULL, NULL, NULL, &dataSize) != ERROR_SUCCESS) {
        ::RegCloseKey(key);
        return QString();
    }

    QByteArray data(dataSize, 0);

    if (::RegQueryValueEx(key, portKeyName, NULL, NULL,
                reinterpret_cast<unsigned char *>(data.data()), &dataSize) != ERROR_SUCCESS) {
        ::RegCloseKey(key);
        return QString();
    }
    ::RegCloseKey(key);
    return QString::fromWCharArray(((const wchar_t *)data.constData()));
}

class SerialPortNameEqualFunctor
{
public:
    explicit SerialPortNameEqualFunctor(const QString &serialPortName)
        : m_serialPortName(serialPortName)
    {
    }

    bool operator() (const QSerialPortInfo &serialPortInfo) const
    {
        return serialPortInfo.portName() == m_serialPortName;
    }

private:
    const QString &m_serialPortName;
};

QList<QSerialPortInfo> QSerialPortInfo::availablePorts()
{
    static const QString usbVendorIdentifierPrefix(QStringLiteral("VID_"));
    static const QString usbProductIdentifierPrefix(QStringLiteral("PID_"));
    static const QString pciVendorIdentifierPrefix(QStringLiteral("VEN_"));
    static const QString pciDeviceIdentifierPrefix(QStringLiteral("DEV_"));

    static const int vendorIdentifierSize = 4;
    static const int productIdentifierSize = 4;

    QList<QSerialPortInfo> serialPortInfoList;

    foreach (const GuidFlagsPair &uniquePair, guidFlagsPairs()) {
        const HDEVINFO deviceInfoSet = ::SetupDiGetClassDevs(reinterpret_cast<const GUID *>(&uniquePair.first), NULL, 0, uniquePair.second);
        if (deviceInfoSet == INVALID_HANDLE_VALUE)
            return serialPortInfoList;

        SP_DEVINFO_DATA deviceInfoData;
        ::memset(&deviceInfoData, 0, sizeof(deviceInfoData));
        deviceInfoData.cbSize = sizeof(deviceInfoData);

        DWORD index = 0;
        while (::SetupDiEnumDeviceInfo(deviceInfoSet, index++, &deviceInfoData)) {
            QSerialPortInfo serialPortInfo;

            QString s = devicePortName(deviceInfoSet, &deviceInfoData);
            if (s.isEmpty() || s.contains(QStringLiteral("LPT")))
                continue;

            if (std::find_if(serialPortInfoList.begin(), serialPortInfoList.end(),
                             SerialPortNameEqualFunctor(s)) != serialPortInfoList.end()) {
                continue;
            }

            serialPortInfo.d_ptr->portName = s;
            serialPortInfo.d_ptr->device = QSerialPortPrivate::portNameToSystemLocation(s);
            serialPortInfo.d_ptr->description =
                    deviceRegistryProperty(deviceInfoSet, &deviceInfoData, SPDRP_DEVICEDESC).toString();
            serialPortInfo.d_ptr->manufacturer =
                    deviceRegistryProperty(deviceInfoSet, &deviceInfoData, SPDRP_MFG).toString();

            s = deviceInstanceIdentifier(deviceInfoSet, &deviceInfoData).toUpper();

            int index = s.indexOf(usbVendorIdentifierPrefix);
            if (index != -1) {
                serialPortInfo.d_ptr->vendorIdentifier = s.mid(index + usbVendorIdentifierPrefix.size(), vendorIdentifierSize)
                        .toInt(&serialPortInfo.d_ptr->hasVendorIdentifier, 16);
            } else {
                index = s.indexOf(pciVendorIdentifierPrefix);
                if (index != -1)
                    serialPortInfo.d_ptr->vendorIdentifier = s.mid(index + pciVendorIdentifierPrefix.size(), vendorIdentifierSize)
                            .toInt(&serialPortInfo.d_ptr->hasVendorIdentifier, 16);
            }

            index = s.indexOf(usbProductIdentifierPrefix);
            if (index != -1) {
                serialPortInfo.d_ptr->productIdentifier = s.mid(index + usbProductIdentifierPrefix.size(), productIdentifierSize)
                        .toInt(&serialPortInfo.d_ptr->hasProductIdentifier, 16);
            } else {
                index = s.indexOf(pciDeviceIdentifierPrefix);
                if (index != -1)
                    serialPortInfo.d_ptr->productIdentifier = s.mid(index + pciDeviceIdentifierPrefix.size(), productIdentifierSize)
                            .toInt(&serialPortInfo.d_ptr->hasProductIdentifier, 16);
            }

            serialPortInfoList.append(serialPortInfo);
        }
        ::SetupDiDestroyDeviceInfoList(deviceInfoSet);
    }
    return serialPortInfoList;
}

#endif

QList<qint32> QSerialPortInfo::standardBaudRates()
{
    return QSerialPortPrivate::standardBaudRates();
}

bool QSerialPortInfo::isBusy() const
{
    const HANDLE descriptor = ::CreateFile(reinterpret_cast<const wchar_t*>(systemLocation().utf16()),
                                           GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

    if (descriptor == INVALID_HANDLE_VALUE) {
        if (::GetLastError() == ERROR_ACCESS_DENIED)
            return true;
    } else {
        ::CloseHandle(descriptor);
    }
    return false;
}

bool QSerialPortInfo::isValid() const
{
    const HANDLE descriptor = ::CreateFile(reinterpret_cast<const wchar_t*>(systemLocation().utf16()),
                                           GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

    if (descriptor == INVALID_HANDLE_VALUE) {
        if (::GetLastError() != ERROR_ACCESS_DENIED)
            return false;
    } else {
        ::CloseHandle(descriptor);
    }
    return true;
}

QT_END_NAMESPACE
