/*
    License...
*/

#include "serialportinfo.h"
#include "serialportinfo_p.h"

#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

extern "C"
{
#include <libudev.h>
}

#include <QtCore/QVariant>
#include <QtCore/QStringList>

//If OS is GNU/Linux
static QStringList nameFilters()
{
    static QStringList list;
    list << "ttyS"    /* standart UART 8250 and etc. */
         << "ttyUSB"  /* usb/serial converters PL2303 and etc. */
         << "ttyACM"  /* CDC_ACM converters (i.e. Mobile Phones). */
         << "ttyMI"   /* MOXA pci/serial converters. */
         << "rfcomm"; /* Bluetooth serial device. */

    return list;
}


/* Public methods */


QList<SerialPortInfo> SerialPortInfo::availablePorts()
{
    QList<SerialPortInfo> ports;

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
                    //get device name
                    QString s(::udev_device_get_devnode(udev_device));

                    foreach (QString mask, nameFilters()) {

                        if (s.contains(mask)) {
                            //name
                            info.d_ptr->portName = s; //< Here add regexp, e.g. for delete /dev/
                            //location
                            info.d_ptr->device = s;
                            //description
                            info.d_ptr->description = QString(::udev_device_get_property_value(udev_device,
                                                                                               "ID_MODEL_FROM_DATABASE"));
                            //manufacturer
                            info.d_ptr->manufacturer = QString(::udev_device_get_property_value(udev_device,
                                                                                                "ID_VENDOR_FROM_DATABASE"));

                            ports.append(info);
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

    return ports;
}

QList<int> SerialPortInfo::standardRates() const
{
    QList<int> rates;

    return rates;
}

bool SerialPortInfo::isBusy() const
{
    // Try open and close port by location: systemLocation();
    //...
    //...

    return false;
}

bool SerialPortInfo::isValid() const
{
    // Impl me
    return false;
}
