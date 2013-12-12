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

#include <QtCore/qlockfile.h>
#include <QtCore/qfile.h>
#include <QtCore/qdir.h>

#ifndef Q_OS_MAC

#include "qtudev_p.h"

#endif

QT_BEGIN_NAMESPACE

#ifndef Q_OS_MAC

static QStringList filteredDeviceFilePaths()
{
    static const QStringList deviceFileNameFilterList = QStringList()

#ifdef Q_OS_LINUX
    << QStringLiteral("ttyS*")    // Standard UART 8250 and etc.
    << QStringLiteral("ttyO*")    // OMAP UART 8250 and etc.
    << QStringLiteral("ttyUSB*")  // Usb/serial converters PL2303 and etc.
    << QStringLiteral("ttyACM*")  // CDC_ACM converters (i.e. Mobile Phones).
    << QStringLiteral("ttyGS*")   // Gadget serial device (i.e. Mobile Phones with gadget serial driver).
    << QStringLiteral("ttyMI*")   // MOXA pci/serial converters.
    << QStringLiteral("ttymxc*")  // Motorola IMX serial ports (i.e. Freescale i.MX).
    << QStringLiteral("ttyAMA*")  // AMBA serial device for embedded platform on ARM (i.e. Raspberry Pi).
    << QStringLiteral("rfcomm*")  // Bluetooth serial device.
    << QStringLiteral("ircomm*"); // IrDA serial device.
#elif defined (Q_OS_FREEBSD)
    << QStringLiteral("cu*");
#else
    ;
#endif

    QStringList result;

    QDir deviceDir(QStringLiteral("/dev"));
    if (deviceDir.exists()) {
        deviceDir.setNameFilters(deviceFileNameFilterList);
        deviceDir.setFilter(QDir::Files | QDir::System | QDir::NoSymLinks);
        QStringList deviceFilePaths;
        foreach (const QFileInfo &deviceFileInfo, deviceDir.entryInfoList()) {
            const QString deviceAbsoluteFilePath = deviceFileInfo.absoluteFilePath();
            if (!deviceFilePaths.contains(deviceAbsoluteFilePath)) {
                deviceFilePaths.append(deviceAbsoluteFilePath);
                result.append(deviceAbsoluteFilePath);
            }
        }
    }

    return result;
}

QList<QSerialPortInfo> availablePortsByFiltersOfDevices()
{
    QList<QSerialPortInfo> serialPortInfoList;

    foreach (const QString &deviceFilePath, filteredDeviceFilePaths()) {
        QSerialPortInfo serialPortInfo;
        serialPortInfo.d_ptr->device = deviceFilePath;
        serialPortInfo.d_ptr->portName = QSerialPortPrivate::portNameFromSystemLocation(deviceFilePath);
        serialPortInfoList.append(serialPortInfo);
    }

    return serialPortInfoList;
}

QList<QSerialPortInfo> availablePortsBySysfs()
{
    QList<QSerialPortInfo> serialPortInfoList;
    QDir ttySysClassDir(QStringLiteral("/sys/class/tty"));
    const bool sysfsEnabled = ttySysClassDir.exists() && ttySysClassDir.isReadable();

    if (sysfsEnabled) {
        ttySysClassDir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
        foreach (const QFileInfo &fileInfo, ttySysClassDir.entryInfoList()) {
            if (!fileInfo.isSymLink())
                continue;

            const QString targetPath = fileInfo.symLinkTarget();
            const int lastIndexOfSlash = targetPath.lastIndexOf(QLatin1Char('/'));
            if (lastIndexOfSlash == -1)
                continue;

            QSerialPortInfo serialPortInfo;

            if (targetPath.contains(QStringLiteral("pnp"))) {
                // TODO: Obtain more information
            } else if (targetPath.contains(QStringLiteral("platform"))) {
                continue;
            } else if (targetPath.contains(QStringLiteral("usb"))) {

                QDir targetDir(targetPath);
                targetDir.setFilter(QDir::Files | QDir::Readable);
                targetDir.setNameFilters(QStringList(QStringLiteral("uevent")));

                do {
                    const QFileInfoList entryInfoList = targetDir.entryInfoList();
                    if (entryInfoList.isEmpty())
                        continue;

                    QFile uevent(entryInfoList.first().absoluteFilePath());
                    if (!uevent.open(QIODevice::ReadOnly | QIODevice::Text))
                        continue;

                    const QString ueventContent = QString::fromLatin1(uevent.readAll());

                    if (ueventContent.contains(QStringLiteral("DEVTYPE=usb_device"))
                            && ueventContent.contains(QStringLiteral("DRIVER=usb"))) {

                        QFile description(QFileInfo(targetDir, QStringLiteral("product")).absoluteFilePath());
                        if (description.open(QIODevice::ReadOnly | QIODevice::Text))
                            serialPortInfo.d_ptr->description = QString::fromLatin1(description.readAll()).simplified();

                        QFile manufacturer(QFileInfo(targetDir, QStringLiteral("manufacturer")).absoluteFilePath());
                        if (manufacturer.open(QIODevice::ReadOnly | QIODevice::Text))
                            serialPortInfo.d_ptr->manufacturer = QString::fromLatin1(manufacturer.readAll()).simplified();

                        QFile vendorIdentifier(QFileInfo(targetDir, QStringLiteral("idVendor")).absoluteFilePath());
                        if (vendorIdentifier.open(QIODevice::ReadOnly | QIODevice::Text)) {
                            serialPortInfo.d_ptr->vendorIdentifier = QString::fromLatin1(vendorIdentifier.readAll())
                                    .toInt(&serialPortInfo.d_ptr->hasVendorIdentifier, 16);
                        }

                        QFile productIdentifier(QFileInfo(targetDir, QStringLiteral("idProduct")).absoluteFilePath());
                        if (productIdentifier.open(QIODevice::ReadOnly | QIODevice::Text)) {
                            serialPortInfo.d_ptr->productIdentifier = QString::fromLatin1(productIdentifier.readAll())
                                    .toInt(&serialPortInfo.d_ptr->hasProductIdentifier, 16);
                        }

                        break;
                    }
                } while (targetDir.cdUp());

            } else if (targetPath.contains(QStringLiteral("pci"))) {
                // TODO: Obtain more information about the device
            } else {
                continue;
            }

            serialPortInfo.d_ptr->portName = targetPath.mid(lastIndexOfSlash + 1);
            serialPortInfo.d_ptr->device = QSerialPortPrivate::portNameToSystemLocation(serialPortInfo.d_ptr->portName);
            serialPortInfoList.append(serialPortInfo);
        }
    }

    return serialPortInfoList;
}

QList<QSerialPortInfo> availablePortsByUdev()
{
#ifndef LINK_LIBUDEV
    static bool symbolsResolved = resolveSymbols();
    if (!symbolsResolved)
        return QList<QSerialPortInfo>();
#endif
    QList<QSerialPortInfo> serialPortInfoList;

    static const QString rfcommDeviceName(QStringLiteral("rfcomm"));

    struct ::udev *udev = ::udev_new();
    if (udev) {

        struct ::udev_enumerate *enumerate =
                ::udev_enumerate_new(udev);

        if (enumerate) {

            ::udev_enumerate_add_match_subsystem(enumerate, "tty");
            ::udev_enumerate_scan_devices(enumerate);

            struct ::udev_list_entry *devices =
                    ::udev_enumerate_get_list_entry(enumerate);

            struct ::udev_list_entry *dev_list_entry;
            udev_list_entry_foreach(dev_list_entry, devices) {

                struct ::udev_device *dev =
                        ::udev_device_new_from_syspath(udev,
                                                       ::udev_list_entry_get_name(dev_list_entry));

                if (dev) {

                    QSerialPortInfo serialPortInfo;

                    serialPortInfo.d_ptr->device = QString::fromLatin1(::udev_device_get_devnode(dev));
                    serialPortInfo.d_ptr->portName = QString::fromLatin1(::udev_device_get_sysname(dev));

                    struct ::udev_device *parentdev = ::udev_device_get_parent(dev);

                    if (parentdev) {

                        QString subsys = QString::fromLatin1(::udev_device_get_subsystem(parentdev));

                        if (subsys == QStringLiteral("usb-serial")
                                || subsys == QStringLiteral("usb")) {
                            serialPortInfo.d_ptr->description = QString::fromLatin1(::udev_device_get_property_value(dev,
                                                                                   "ID_MODEL")).replace(QLatin1Char('_'), QLatin1Char(' '));
                            serialPortInfo.d_ptr->manufacturer = QString::fromLatin1(::udev_device_get_property_value(dev,
                                                                                   "ID_VENDOR")).replace(QLatin1Char('_'), QLatin1Char(' '));

                            serialPortInfo.d_ptr->vendorIdentifier =
                                    QString::fromLatin1(::udev_device_get_property_value(dev,
                                                "ID_VENDOR_ID")).toInt(&serialPortInfo.d_ptr->hasVendorIdentifier, 16);

                            serialPortInfo.d_ptr->productIdentifier =
                                    QString::fromLatin1(::udev_device_get_property_value(dev,
                                                "ID_MODEL_ID")).toInt(&serialPortInfo.d_ptr->hasProductIdentifier, 16);

                        } else if (subsys == QStringLiteral("pnp")) {
                            // TODO: Obtain more information
                        } else if (subsys == QStringLiteral("platform")) {
                            continue;
                        } else if (subsys == QStringLiteral("pci")) {
                            // TODO: Obtain more information about the device
                        } else {
                            // FIXME: Obtain more information
                        }
                    } else {
                        if (serialPortInfo.d_ptr->portName.startsWith(rfcommDeviceName)) {
                            bool ok;
                            int portNumber = serialPortInfo.d_ptr->portName.mid(rfcommDeviceName.length()).toInt(&ok);

                            if (!ok || (portNumber < 0) || (portNumber > 255))
                                continue;
                        } else {
                            continue;
                        }
                    }

                    serialPortInfoList.append(serialPortInfo);

                    ::udev_device_unref(dev);
                }

            }

            ::udev_enumerate_unref(enumerate);
        }

        ::udev_unref(udev);
    }

    return serialPortInfoList;
}

QList<QSerialPortInfo> QSerialPortInfo::availablePorts()
{
    QList<QSerialPortInfo> serialPortInfoList;
    // TODO: Remove this condition once the udev runtime symbol resolution crash
    // is fixed for Qt 4.
#if defined(LINK_LIBUDEV) || (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    serialPortInfoList = availablePortsByUdev();
#endif

#ifdef Q_OS_LINUX
    if (serialPortInfoList.isEmpty())
        serialPortInfoList = availablePortsBySysfs();
    else
        return serialPortInfoList;
#endif

    if (serialPortInfoList.isEmpty())
        serialPortInfoList = availablePortsByFiltersOfDevices();

    return serialPortInfoList;
}

#endif

QList<qint32> QSerialPortInfo::standardBaudRates()
{
    return QSerialPortPrivate::standardBaudRates();
}

bool QSerialPortInfo::isBusy() const
{
    QString lockFilePath = serialPortLockFilePath(portName());
    if (lockFilePath.isEmpty())
        return false;

    QLockFile lockFile(lockFilePath);
    return lockFile.isLocked();
}

bool QSerialPortInfo::isValid() const
{
    QFile f(systemLocation());
    return f.exists();
}

QT_END_NAMESPACE
