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
#include <sys/ioctl.h>
#include <fcntl.h>

#if defined (Q_OS_LINUX)
#  include <linux/serial.h>
#endif

#if defined (HAVE_UDEV)
extern "C"
{
#  include <libudev.h>
}
#else
#  include <QtCore/qdir.h>
#endif

#include <QtCore/qvariant.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qregexp.h>
#include <QtCore/qfile.h>

QT_USE_NAMESPACE

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

                            info.d_ptr->description =
                                    QString(::udev_device_get_property_value(udev_device,
                                                                             "ID_MODEL_FROM_DATABASE"));
                            if (info.d_ptr->description.isEmpty())
                                info.d_ptr->description = QString(QObject::tr("Unknown."));

                            info.d_ptr->manufacturer =
                                    QString(::udev_device_get_property_value(udev_device,
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

static
const qint32 standardRates_begin[] =
{
    50, 75, 110, 134, 150,
    200, 300, 600, 1200, 1800,
    2400, 4800, 9600, 19200, 38400,
    57600, 115200, 230400, 460800,
    500000, 576000, 921600, 1000000,
    1152000, 1500000, 2000000, 2500000,
    3000000, 3500000, 4000000
}, *standardRates_end = standardRates_begin + sizeof(::standardRates_begin)/sizeof(*::standardRates_begin);

QList<qint32> SerialPortInfo::standardRates() const
{
    QList<qint32> rates;
    rates.reserve(standardRates_end - standardRates_begin);

#if defined (Q_OS_LINUX) && defined (TIOCGSERIAL) && defined (TIOCSSERIAL)
    int descriptor = ::open(systemLocation().toLocal8Bit().constData(),
                            O_NOCTTY | O_NDELAY | O_RDWR);
    if (descriptor != -1) {

        struct serial_struct ser_info;
        int result = ::ioctl(descriptor, TIOCGSERIAL, &ser_info);

        ::close(descriptor);

        if ((result != -1) && (ser_info.baud_base > 0)) {

            for (int i = 1, max = ser_info.baud_base / 50; i <= max; ++i) {
                result = ser_info.baud_base / i;

                //append to list only rates presented in array standardRates_begin
                if ((qBinaryFind(standardRates_begin, standardRates_end, result) != standardRates_end) &&
                        !rates.contains(result))
                    rates.append(result);
            }
        }
    }
#else
    for (const qint32 *i = standardRates_begin; i != standardRates_end; ++i)
        rates.append(*i);
#endif
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
    QFile f(systemLocation());
    return f.exists();
}
