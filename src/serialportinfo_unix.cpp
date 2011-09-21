/*
    License...
*/

#include "serialportinfo.h"
#include "serialportinfo_p.h"
#include "ttylocker_p_unix.h"

#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

#if defined (HAVE_UDEV)
extern "C"
{
#  include <libudev.h>
}
#else
#  include <QDir>
#endif

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QRegExp>

static QStringList nameFilters()
{
    static QStringList list;
#if defined (Q_OS_LINUX)
    list << "ttyS*"    /* Standart UART 8250 and etc. */
         << "ttyUSB*"  /* Usb/serial converters PL2303 and etc. */
         << "ttyACM*"  /* CDC_ACM converters (i.e. Mobile Phones). */
         << "ttyMI*"   /* MOXA pci/serial converters. */
         << "rfcomm*"; /* Bluetooth serial device. */
#else
    // Here for other *nix OS.
#endif
    return list;
}


/* Public methods */


QList<SerialPortInfo> SerialPortInfo::availablePorts()
{
    QList<SerialPortInfo> ports;

#if defined (HAVE_UDEV)
    // With udev scan.
    struct udev *udev = ::udev_new();
    if (udev) {

        struct udev_enumerate *enumerate = ::udev_enumerate_new(udev);
        if (enumerate) {

            ::udev_enumerate_add_match_subsystem(enumerate, "tty");
            ::udev_enumerate_scan_devices(enumerate);

            struct udev_list_entry *devices = ::udev_enumerate_get_list_entry(enumerate);

            struct udev_list_entry *dev_list_entry;
            udev_list_entry_foreach(dev_list_entry, devices) {

                const char *syspath = ::udev_list_entry_get_name(dev_list_entry);
                struct udev_device *udev_device = ::udev_device_new_from_syspath(udev, syspath);

                if (udev_device) {

                    SerialPortInfo info;
                    QString s(::udev_device_get_devnode(udev_device));

                    foreach (QString mask, nameFilters()) {

                        if (s.contains(mask.remove("*"))) {
                            info.d_ptr->device = s;
                            info.d_ptr->portName = s.remove(QRegExp("/[\\w|\\d|\\s]+/"));

                            info.d_ptr->description = QString(::udev_device_get_property_value(udev_device,
                                                                                               "ID_MODEL_FROM_DATABASE"));
                            if (info.d_ptr->description.isEmpty())
                                info.d_ptr->description = QString(QObject::tr("Unknown."));

                            info.d_ptr->manufacturer = QString(::udev_device_get_property_value(udev_device,
                                                                                                "ID_VENDOR_FROM_DATABASE"));
                            if (info.d_ptr->manufacturer.isEmpty())
                                info.d_ptr->manufacturer = QString(QObject::tr("Unknown."));

                            ports.append(info);
                            break;
                        }
                    }
                    ::udev_device_unref(udev_device);
                }
            }
        }

        if (enumerate)
            ::udev_enumerate_unref(enumerate);
    }

    if (udev)
        ::udev_unref(udev);
#else
    // With directory /dev scan.
    QDir devDir("/dev");
    if (devDir.exists()) {

        devDir.setNameFilters(nameFilters());
        devDir.setFilter(QDir::Files | QDir::System);

        foreach(QFileInfo fi, devDir.entryInfoList()) {
            if (!fi.isDir()) {

                SerialPortInfo info;

                info.d_ptr->device = fi.absoluteFilePath();
                info.d_ptr->portName = info.d_ptr->device;
                info.d_ptr->portName.remove(QRegExp("/[\\w|\\d|\\s]+/"));
                info.d_ptr->description = QString(QObject::tr("Unknown."));
                info.d_ptr->manufacturer = QString(QObject::tr("Unknown."));

                ports.append(info);
            }
        }
    }
#endif

    return ports;
}

QList<qint32> SerialPortInfo::standardRates() const
{
    QList<qint32> rates;

    return rates;
}

bool SerialPortInfo::isBusy() const
{
    bool currPid = false;
    bool ret = TTYLocker::isLocked(systemLocation(), &currPid);
    return ret;
}

bool SerialPortInfo::isValid() const
{
    // Impl me
    return false;
}
