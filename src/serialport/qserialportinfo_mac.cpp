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
#include "qserialport_unix_p.h"

#include "private/qcore_mac_p.h"

#include <sys/param.h>

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>

#include <IOKit/serial/IOSerialKeys.h>
#include <IOKit/storage/IOStorageDeviceCharacteristics.h> // for kIOPropertyProductNameKey
#include <IOKit/usb/USB.h>
#if defined(MAC_OS_X_VERSION_10_4) && (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4)
#  include <IOKit/serial/ioss.h>
#endif
#include <IOKit/IOBSD.h>

QT_BEGIN_NAMESPACE

static QCFType<CFTypeRef> searchProperty(io_registry_entry_t ioRegistryEntry,
                                         const QCFString &propertyKey)
{
    return ::IORegistryEntrySearchCFProperty(
                ioRegistryEntry, kIOServicePlane, propertyKey, kCFAllocatorDefault, 0);
}

static QString searchStringProperty(io_registry_entry_t ioRegistryEntry,
                                    const QCFString &propertyKey)
{
    const QCFString result(searchProperty(ioRegistryEntry, propertyKey).as<CFStringRef>());
    return QCFString::toQString(result);
}

static quint16 searchShortIntProperty(io_registry_entry_t ioRegistryEntry,
                                      const QCFString &propertyKey,
                                      bool &ok)
{
    const QCFType<CFTypeRef> result(searchProperty(ioRegistryEntry, propertyKey));
    quint16 value = 0;
    ok = result.as<CFNumberRef>()
            && (::CFNumberGetValue(result.as<CFNumberRef>(), kCFNumberShortType, &value) > 0);
    return value;
}

static bool isCompleteInfo(const QSerialPortInfo &portInfo)
{
    return !portInfo.portName().isEmpty()
            && !portInfo.systemLocation().isEmpty()
            && !portInfo.manufacturer().isEmpty()
            && !portInfo.description().isEmpty()
            && !portInfo.serialNumber().isEmpty()
            && portInfo.hasProductIdentifier()
            && portInfo.hasVendorIdentifier();
}

static QString devicePortName(io_registry_entry_t ioRegistryEntry)
{
    return searchStringProperty(ioRegistryEntry, QCFString(kIOTTYDeviceKey));
}

static QString deviceSystemLocation(io_registry_entry_t ioRegistryEntry)
{
    return searchStringProperty(ioRegistryEntry, QCFString(kIOCalloutDeviceKey));
}

static QString deviceDescription(io_registry_entry_t ioRegistryEntry)
{
    QString result = searchStringProperty(ioRegistryEntry, QCFString(kIOPropertyProductNameKey));
    if (result.isEmpty())
        result = searchStringProperty(ioRegistryEntry, QCFString(kUSBProductString));
    if (result.isEmpty())
        result = searchStringProperty(ioRegistryEntry, QCFString("BTName"));
    return result;
}

static QString deviceManufacturer(io_registry_entry_t ioRegistryEntry)
{
    return searchStringProperty(ioRegistryEntry, QCFString(kUSBVendorString));
}

static QString deviceSerialNumber(io_registry_entry_t ioRegistryEntry)
{
    return searchStringProperty(ioRegistryEntry, QCFString(kUSBSerialNumberString));
}

static quint16 deviceVendorIdentifier(io_registry_entry_t ioRegistryEntry, bool &ok)
{
    return searchShortIntProperty(ioRegistryEntry, QCFString(kUSBVendorID), ok);
}

static quint16 deviceProductIdentifier(io_registry_entry_t ioRegistryEntry, bool &ok)
{
    return searchShortIntProperty(ioRegistryEntry, QCFString(kUSBProductID), ok);
}

static io_registry_entry_t parentSerialPortService(io_registry_entry_t currentSerialPortService)
{
    io_registry_entry_t result = 0;
    ::IORegistryEntryGetParentEntry(currentSerialPortService, kIOServicePlane, &result);
    ::IOObjectRelease(currentSerialPortService);
    return result;
}

QList<QSerialPortInfo> QSerialPortInfo::availablePorts()
{
    CFMutableDictionaryRef serialPortDictionary = ::IOServiceMatching(kIOSerialBSDServiceValue);
    if (!serialPortDictionary)
        return QList<QSerialPortInfo>();

    ::CFDictionaryAddValue(serialPortDictionary,
                           CFSTR(kIOSerialBSDTypeKey),
                           CFSTR(kIOSerialBSDAllTypes));

    io_iterator_t serialPortIterator = 0;
    if (::IOServiceGetMatchingServices(kIOMasterPortDefault, serialPortDictionary,
                                       &serialPortIterator) != KERN_SUCCESS) {
        return QList<QSerialPortInfo>();
    }

    QList<QSerialPortInfo> serialPortInfoList;

    forever {
        io_registry_entry_t serialPortService = ::IOIteratorNext(serialPortIterator);
        if (!serialPortService)
            break;

        QSerialPortInfo serialPortInfo;

        forever {
            if (serialPortInfo.portName().isEmpty())
                serialPortInfo.d_ptr->portName = devicePortName(serialPortService);

            if (serialPortInfo.systemLocation().isEmpty())
                serialPortInfo.d_ptr->device = deviceSystemLocation(serialPortService);

            if (serialPortInfo.description().isEmpty())
                serialPortInfo.d_ptr->description = deviceDescription(serialPortService);

            if (serialPortInfo.manufacturer().isEmpty())
                serialPortInfo.d_ptr->manufacturer = deviceManufacturer(serialPortService);

            if (serialPortInfo.serialNumber().isEmpty())
                serialPortInfo.d_ptr->serialNumber = deviceSerialNumber(serialPortService);

            if (!serialPortInfo.hasVendorIdentifier()) {
                serialPortInfo.d_ptr->vendorIdentifier =
                        deviceVendorIdentifier(serialPortService,
                                               serialPortInfo.d_ptr->hasVendorIdentifier);
            }

            if (!serialPortInfo.hasProductIdentifier()) {
                serialPortInfo.d_ptr->productIdentifier =
                        deviceProductIdentifier(serialPortService,
                                                serialPortInfo.d_ptr->hasProductIdentifier);
            }

            if (isCompleteInfo(serialPortInfo)) {
                ::IOObjectRelease(serialPortService);
                break;
            }

            serialPortService = parentSerialPortService(serialPortService);
            if (!serialPortService)
                break;
        }

        serialPortInfoList.append(serialPortInfo);
    }

    ::IOObjectRelease(serialPortIterator);

    return serialPortInfoList;
}

QList<qint32> QSerialPortInfo::standardBaudRates()
{
    return QSerialPortPrivate::standardBaudRates();
}

bool QSerialPortInfo::isBusy() const
{
    QString lockFilePath = serialPortLockFilePath(portName());
    if (lockFilePath.isEmpty())
        return false;

    QFile reader(lockFilePath);
    if (!reader.open(QIODevice::ReadOnly))
        return false;

    QByteArray pidLine = reader.readLine();
    pidLine.chop(1);
    if (pidLine.isEmpty())
        return false;

    qint64 pid = pidLine.toLongLong();

    if (pid && (::kill(pid, 0) == -1) && (errno == ESRCH))
        return false; // PID doesn't exist anymore

    return true;
}

bool QSerialPortInfo::isValid() const
{
    QFile f(systemLocation());
    return f.exists();
}

QT_END_NAMESPACE
