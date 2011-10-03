/*
    License...
*/

#include "serialportinfo.h"
#include "serialportinfo_p.h"
#include "ttylocker_p_unix.h"

#include <sys/param.h>

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>

#include <IOKit/serial/IOSerialKeys.h>
#if defined(MAC_OS_X_VERSION_10_4) && (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4)
#  include <IOKit/serial/ioss.h>
#endif
#include <IOKit/IOBSD.h>

#include <QtCore/qvariant.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qfile.h>

QT_USE_NAMESPACE

/* Public methods */


QList<SerialPortInfo> SerialPortInfo::availablePorts()
{
    QList<SerialPortInfo> ports;

    CFMutableDictionaryRef classesToMatch = IOServiceMatching(kIOSerialBSDServiceValue);
    if (!classesToMatch)
        return ports;

    CFDictionaryAddValue(classesToMatch,
                         CFSTR(kIOSerialBSDTypeKey),
                         CFSTR(kIOSerialBSDAllTypes));

    io_iterator_t serialPortIterator = 0;
    kern_return_t kernResult = IOServiceGetMatchingServices(kIOMasterPortDefault,
                                                            classesToMatch,
                                                            &serialPortIterator);
    if (kernResult == KERN_SUCCESS) {

        io_object_t serialService;
        while ((serialService = IOIteratorNext(serialPortIterator))) {

            SerialPortInfo info;

            //Device name
            CFTypeRef nameRef = IORegistryEntryCreateCFProperty(serialService,
                                                                CFSTR(kIOCalloutDeviceKey),
                                                                kCFAllocatorDefault,
                                                                0);
            bool result = false;
            QByteArray bsdPath(MAXPATHLEN, 0);
            if (nameRef) {
                result = CFStringGetCString(CFStringRef(nameRef), bsdPath.data(), bsdPath.size(),
                                            kCFStringEncodingUTF8);
                CFRelease(nameRef);
            }

            // If device name (path) found.
            if (result) {

                io_registry_entry_t parent;
                kernResult = IORegistryEntryGetParentEntry(serialService, kIOServicePlane, &parent);

                CFTypeRef descriptionRef = 0;
                CFTypeRef manufacturerRef = 0;

                //
                while ((kernResult == KERN_SUCCESS)) {

                    if (!descriptionRef)
                        descriptionRef = IORegistryEntrySearchCFProperty(parent, kIOServicePlane,
                                                                         CFSTR("USB Product Name"),
                                                                         kCFAllocatorDefault, 0);
                    if (!manufacturerRef)
                        manufacturerRef = IORegistryEntrySearchCFProperty(parent, kIOServicePlane,
                                                                          CFSTR("USB Vendor Name"),
                                                                          kCFAllocatorDefault, 0);

                    ////// Next info
                    //.....

                    io_registry_entry_t currParent = parent;
                    kernResult = IORegistryEntryGetParentEntry(currParent, kIOServicePlane, &parent);
                    IOObjectRelease(currParent);
                }

                QByteArray stringValue(MAXPATHLEN, 0);

                QString s(bsdPath);
                info.d_ptr->device = s;
                info.d_ptr->portName = s.remove(QRegExp("/[\\w|\\d|\\s]+/"));

                if (descriptionRef) {
                    if (CFStringGetCString(CFStringRef(descriptionRef),
                                           stringValue.data(), stringValue.size(),
                                           kCFStringEncodingUTF8)) {

                        info.d_ptr->description = QString(stringValue);
                    }
                    CFRelease(descriptionRef);
                }
                if (info.d_ptr->description.isEmpty())
                    info.d_ptr->description = QString(QObject::tr("Unknown."));

                if (manufacturerRef) {
                    if (CFStringGetCString(CFStringRef(manufacturerRef),
                                           stringValue.data(), stringValue.size(),
                                           kCFStringEncodingUTF8)) {

                        info.d_ptr->manufacturer = QString(stringValue);
                    }
                    CFRelease(manufacturerRef);
                }
                if (info.d_ptr->manufacturer.isEmpty())
                    info.d_ptr->manufacturer = QString(QObject::tr("Unknown."));

                ports.append(info);
            }
            (void) IOObjectRelease(serialService);
        }
    }

    // Here delete classetToMach ?

    return ports;
}

QList<qint32> SerialPortInfo::standardRates() const
{
    QList<qint32> rates;

    /*
      MacX implementation detect supported rates list
      or append standart Unix rates.
    */

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
