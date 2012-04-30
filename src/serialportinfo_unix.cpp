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
#include "ttylocker_unix_p.h"
#include "serialportengine_unix_p.h"

#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#if defined (Q_OS_LINUX)
#  include <linux/serial.h>
#endif

#if defined (Q_OS_LINUX) && defined (HAVE_LIBUDEV)
extern "C"
{
#  include <libudev.h>
}
#else
#  include <QtCore/qdir.h>
#endif

#include <QtCore/qstringlist.h>
#include <QtCore/qregexp.h>
#include <QtCore/qfile.h>



#if defined (Q_OS_LINUX) && defined (HAVE_LIBUDEV)

// For detail enumerate - skipping, filters is not used.
// Instead of filters used libudev.

#else

// For simple enumerate.

static QStringList generateFiltersOfDevices()
{
    QStringList l;

#  if defined (Q_OS_LINUX)
    l << QLatin1String("ttyS*")    // Standart UART 8250 and etc.
      << QLatin1String("ttyUSB*")  // Usb/serial converters PL2303 and etc.
      << QLatin1String("ttyACM*")  // CDC_ACM converters (i.e. Mobile Phones).
      << QLatin1String("ttyMI*")   // MOXA pci/serial converters.
      << QLatin1String("rfcomm*"); // Bluetooth serial device.
#  elif defined (Q_OS_FREEBSD)
    l << QLatin1String("cu*");
#  else
    // Here for other *nix OS.
#  endif

    return l;
}

// Only for a simple enumeration by the mask of all available
// devices in /dev directory. Used in the following cases:
// - for Gnu/Linux without libudev
// - for any other *nix, bsd (exception Mac OSX)
inline QStringList& filtersOfDevices()
{
    static QStringList l = generateFiltersOfDevices();
    return l;
}

#endif

QT_BEGIN_NAMESPACE_SERIALPORT


/* Public methods */


QList<SerialPortInfo> SerialPortInfo::availablePorts()
{
    QList<SerialPortInfo> ports;

#if defined (Q_OS_LINUX) && defined (HAVE_LIBUDEV)

    // Detailed enumerate devices for Gnu/Linux with use libudev.

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

                    SerialPortInfo info;

                    info.d_ptr->device =
                            QString::fromLatin1(::udev_device_get_devnode(dev));
                    info.d_ptr->portName =
                            QString::fromLatin1(::udev_device_get_sysname(dev));

                    struct ::udev_device *parentdev = ::udev_device_get_parent(dev);

                    if (parentdev) {

                        QString subsys(QLatin1String(::udev_device_get_subsystem(parentdev)));

                        bool do_append = true;

                        if (subsys.contains(QLatin1String("usb"))) {
                            info.d_ptr->description =
                                    QString::fromLatin1(::udev_device_get_property_value(dev,
                                                                                         "ID_MODEL_FROM_DATABASE"));
                            info.d_ptr->manufacturer =
                                    QString::fromLatin1(::udev_device_get_property_value(dev,
                                                                                         "ID_VENDOR_FROM_DATABASE"));
                            info.d_ptr->vendorIdentifier =
                                    QString::fromLatin1(::udev_device_get_property_value(dev,
                                                                                         "ID_VENDOR_ID"));
                            info.d_ptr->productIdentifier =
                                    QString::fromLatin1(::udev_device_get_property_value(dev,
                                                                                         "ID_MODEL_ID"));

                        } else if (subsys.contains(QLatin1String("pnp"))) {
                            // FIXME: How can I get a additional info about standard serial devices?

                        } else {
                            // Others subsystems.
                            // Skip, while don't use.
                            do_append = false;
                        }

                        if (do_append)
                            ports.append(info);
                    }

                    ::udev_device_unref(dev);
                }

            }

            ::udev_enumerate_unref(enumerate);
        }

        ::udev_unref(udev);
    }

#elif defined (Q_OS_FREEBSD) && defined (HAVE_LIBUSB)

    // TODO: Implement me.
    //

#else

    // Simple enumerate devices for other *nix OS in device directory /dev.

    QDir devDir(QLatin1String("/dev"));
    if (devDir.exists()) {

        devDir.setNameFilters(filtersOfDevices());
        devDir.setFilter(QDir::Files | QDir::System);

        QStringList foundDevices; // Found devices list.

        foreach (const QFileInfo &fi, devDir.entryInfoList()) {
            if (!fi.isDir()) {

                QString s = fi.absoluteFilePath().split('.').at(0);
                if (!foundDevices.contains(s)) {
                    foundDevices.append(s);

                    SerialPortInfo info;

                    info.d_ptr->device = s;
                    info.d_ptr->portName = s.remove(QRegExp(QLatin1String("/[\\w|\\d|\\s]+/")));

                    // Get description, manufacturer, vendor identifier, product
                    // identifier are not supported.

                    ports.append(info);

                }
            }
        }
    }

#endif

    return ports;
}

QList<qint32> SerialPortInfo::standardRates()
{
    return UnixSerialPortEngine::standardRates();
}

bool SerialPortInfo::isBusy() const
{
    bool currentPid = false;
    bool ret = TTYLocker::isLocked(systemLocation(), &currentPid);
    return ret;
}

bool SerialPortInfo::isValid() const
{
    QFile f(systemLocation());
    return f.exists();
}

QT_END_NAMESPACE_SERIALPORT
