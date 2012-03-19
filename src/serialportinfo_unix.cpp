/*
    License...
*/

#include "serialportinfo.h"
#include "serialportinfo_p.h"
#include "ttylocker_unix_p.h"

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
                            info.d_ptr->vid =
                                    QString::fromLatin1(::udev_device_get_property_value(dev,
                                                                                         "ID_VENDOR_ID"));
                            info.d_ptr->pid =
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

                    // Get description, manufacturer, vid, pid is not supported.

                    ports.append(info);

                }
            }
        }
    }

#endif

    return ports;
}

QList<qint32> SerialPortInfo::standardRates() const
{
    QList<qint32> rates;
    rates.reserve(standardRates_end - standardRates_begin);

#if defined (Q_OS_LINUX) && defined (TIOCGSERIAL) && defined (TIOCSSERIAL)

    int descriptor = ::open(systemLocation().toLocal8Bit().constData(),
                            O_NOCTTY | O_NDELAY | O_RDWR);
    if (descriptor != -1) {

        struct serial_struct ser_info;

        // This call with may TIOCGSERIAL be not supported on some
        // USB/Serial converters (for example, PL2303).
        int result = ::ioctl(descriptor, TIOCGSERIAL, &ser_info);
        ::close(descriptor);
        if ((result != -1) && (ser_info.baud_base > 0)) {

            for (int i = 1, max = ser_info.baud_base / 50; i <= max; ++i) {
                result = ser_info.baud_base / i;

                //append to list only rates presented in array standardRates_begin
                if ((qBinaryFind(standardRates_begin, standardRates_end, result) != standardRates_end)
                        && !rates.contains(result)) {
                    rates.append(result);
                }
            }
        }
    }

#else

    for (const qint32 *i = standardRates_begin; i != standardRates_end; ++i)
        rates.append(*i);

#endif

    qSort(rates);
    return rates;
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
