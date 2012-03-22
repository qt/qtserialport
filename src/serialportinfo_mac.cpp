/*
    License...
*/

#include "serialportinfo.h"
#include "serialportinfo_p.h"
#include "ttylocker_unix_p.h"
#include "serialportengine_unix_p.h"

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

#include <QtCore/qvariant.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qfile.h>

QT_BEGIN_NAMESPACE_SERIALPORT

/* Public methods */

//
enum { MATCHING_PROPERTIES_COUNT = 6 };

QList<SerialPortInfo> SerialPortInfo::availablePorts()
{
    QList<SerialPortInfo> ports;

    int matchingPropertiesCounter = 0;


    ::CFMutableDictionaryRef matching = ::IOServiceMatching(kIOSerialBSDServiceValue);

    ::CFDictionaryAddValue(matching,
                           CFSTR(kIOSerialBSDTypeKey),
                           CFSTR(kIOSerialBSDAllTypes));

    io_iterator_t iter = 0;
    kern_return_t kr = ::IOServiceGetMatchingServices(kIOMasterPortDefault,
                                                      matching,
                                                      &iter);

    if (kr != kIOReturnSuccess)
        return ports;

    io_registry_entry_t service;

    while ((service = ::IOIteratorNext(iter))) {

        ::CFTypeRef device = 0;
        ::CFTypeRef portName = 0;
        ::CFTypeRef description = 0;
        ::CFTypeRef manufacturer = 0;
        ::CFTypeRef vid = 0;
        ::CFTypeRef pid = 0;

        io_registry_entry_t entry = service;

        // Find MacOSX-specific properties names.
        do {

            if (!device) {
                device =
                        ::IORegistryEntrySearchCFProperty(entry,
                                                          kIOServicePlane,
                                                          CFSTR(kIOCalloutDeviceKey),
                                                          kCFAllocatorDefault,
                                                          0);
                if (device)
                    ++matchingPropertiesCounter;
            }

            if (!portName) {
                portName =
                        ::IORegistryEntrySearchCFProperty(entry,
                                                          kIOServicePlane,
                                                          CFSTR(kIOTTYDeviceKey),
                                                          kCFAllocatorDefault,
                                                          0);
                if (portName)
                    ++matchingPropertiesCounter;
            }

            if (!description) {
                description =
                        ::IORegistryEntrySearchCFProperty(entry,
                                                          kIOServicePlane,
                                                          CFSTR(kIOPropertyProductNameKey),
                                                          kCFAllocatorDefault,
                                                          0);
                if (!description)
                    description =
                            ::IORegistryEntrySearchCFProperty(entry,
                                                              kIOServicePlane,
                                                              CFSTR(kUSBProductString),
                                                              kCFAllocatorDefault,
                                                              0);
                if (description)
                    ++matchingPropertiesCounter;
            }

            if (!manufacturer) {
                manufacturer =
                        ::IORegistryEntrySearchCFProperty(entry,
                                                          kIOServicePlane,
                                                          CFSTR(kUSBVendorString),
                                                          kCFAllocatorDefault,
                                                          0);
                if (manufacturer)
                    ++matchingPropertiesCounter;

            }

            if (!vid) {
                vid =
                        ::IORegistryEntrySearchCFProperty(entry,
                                                          kIOServicePlane,
                                                          CFSTR(kUSBVendorID),
                                                          kCFAllocatorDefault,
                                                          0);
                if (vid)
                    ++matchingPropertiesCounter;

            }

            if (!pid) {
                pid =
                        ::IORegistryEntrySearchCFProperty(entry,
                                                          kIOServicePlane,
                                                          CFSTR(kUSBProductID),
                                                          kCFAllocatorDefault,
                                                          0);
                if (pid)
                    ++matchingPropertiesCounter;

            }

            // If all matching properties is found, then force break loop.
            if (matchingPropertiesCounter == MATCHING_PROPERTIES_COUNT)
                break;

            kr = ::IORegistryEntryGetParentEntry(entry, kIOServicePlane, &entry);

        } while (kr == kIOReturnSuccess);

        (void) ::IOObjectRelease(entry);

        // Convert from MacOSX-specific properties to Qt4-specific.
        if (matchingPropertiesCounter > 0) {

            SerialPortInfo info;
            QByteArray buffer(MAXPATHLEN, 0);

            if (device) {
                if (::CFStringGetCString(CFStringRef(device),
                                         buffer.data(),
                                         buffer.size(),
                                         kCFStringEncodingUTF8)) {

                    info.d_ptr->device = QString(buffer);
                }

                ::CFRelease(device);
            }

            if (portName) {
                if (::CFStringGetCString(CFStringRef(portName),
                                         buffer.data(),
                                         buffer.size(),
                                         kCFStringEncodingUTF8)) {

                    info.d_ptr->portName = QString(buffer);
                }

                ::CFRelease(portName);
            }

            if (description) {
                ::CFStringGetCString(CFStringRef(description),
                                     buffer.data(),
                                     buffer.size(),
                                     kCFStringEncodingUTF8);

                info.d_ptr->description = QString(buffer);
                ::CFRelease(description);
            }

            if (manufacturer) {
                ::CFStringGetCString(CFStringRef(manufacturer),
                                     buffer.data(),
                                     buffer.size(),
                                     kCFStringEncodingUTF8);

                info.d_ptr->manufacturer = QString(buffer);
                ::CFRelease(manufacturer);
            }


            int value = 0;

            if (vid) {
                ::CFNumberGetValue(CFNumberRef(vid),
                                   kCFNumberIntType,
                                   &value);

                info.d_ptr->vid = QString::number(value, 16);
                ::CFRelease(vid);
            }

            if (pid) {
                ::CFNumberGetValue(CFNumberRef(pid),
                                   kCFNumberIntType,
                                   &value);

                info.d_ptr->pid = QString::number(value, 16);
                ::CFRelease(pid);
            }

            ports.append(info);
        }

        (void) ::IOObjectRelease(service);
    }

    (void) ::IOObjectRelease(iter);

    return ports;
}

static QList<qint32> emptyList()
{
    QList<qint32> list;
    return list;
}

QList<qint32> SerialPortInfo::standardRates() const
{
    return UnixSerialPortEngine::standardRates();
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

QT_END_NAMESPACE_SERIALPORT
