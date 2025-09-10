// Copyright (C) 2011-2012 Denis Shienkov <denis.shienkov@gmail.com>
// Copyright (C) 2011 Sergey Belyashov <Sergey.Belyashov@gmail.com>
// Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qserialportinfo.h"
#include "qserialportinfo_p.h"
#include "qserialport_p.h"

#include <QtCore/qlockfile.h>
#include <QtCore/qfile.h>
#include <QtCore/qdir.h>
#include <QtCore/qtenvironmentvariables.h>

#include <private/qcore_unix_p.h>

#include <memory>

#include <errno.h>
#include <sys/types.h> // kill
#include <signal.h>    // kill

#include "qtudev_p.h"

QT_BEGIN_NAMESPACE

constexpr char SkipUdevEnvVarName[] = "QT_SERIALPORT_SKIP_UDEV_LOOKUP";

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
    << QStringLiteral("ttyTHS*")  // Serial device for embedded platform on ARM (i.e. Tegra Jetson TK1).
    << QStringLiteral("rfcomm*")  // Bluetooth serial device.
    << QStringLiteral("ircomm*")  // IrDA serial device.
    << QStringLiteral("tnt*");    // Virtual tty0tty serial device.
#elif defined(Q_OS_FREEBSD)
    << QStringLiteral("cu*");
#elif defined(Q_OS_QNX)
    << QStringLiteral("ser*");
#else
    ;
#endif

    QStringList result;

    QDir deviceDir(QStringLiteral("/dev"));
    if (deviceDir.exists()) {
        deviceDir.setNameFilters(deviceFileNameFilterList);
        deviceDir.setFilter(QDir::Files | QDir::System | QDir::NoSymLinks);
        QStringList deviceFilePaths;
        const auto deviceFileInfos = deviceDir.entryInfoList();
        for (const QFileInfo &deviceFileInfo : deviceFileInfos) {
            const QString deviceAbsoluteFilePath = deviceFileInfo.absoluteFilePath();

#ifdef Q_OS_FREEBSD
            // it is a quick workaround to skip the non-serial devices
            if (deviceAbsoluteFilePath.endsWith(QLatin1String(".init"))
                    || deviceAbsoluteFilePath.endsWith(QLatin1String(".lock"))) {
                continue;
            }
#endif

            if (!deviceFilePaths.contains(deviceAbsoluteFilePath)) {
                deviceFilePaths.append(deviceAbsoluteFilePath);
                result.append(deviceAbsoluteFilePath);
            }
        }
    }

    return result;
}

QList<QSerialPortInfo> availablePortsByFiltersOfDevices(bool &ok)
{
    QList<QSerialPortInfo> serialPortInfoList;

    const auto deviceFilePaths = filteredDeviceFilePaths();
    for (const QString &deviceFilePath : deviceFilePaths) {
        QSerialPortInfoPrivate priv;
        priv.device = deviceFilePath;
        priv.portName = QSerialPortInfoPrivate::portNameFromSystemLocation(deviceFilePath);
        serialPortInfoList.append(priv);
    }

    ok = true;
    return serialPortInfoList;
}

static bool isSerial8250Driver(const QString &driverName)
{
    return (driverName == QLatin1String("serial8250"));
}

static bool isValidSerial8250(const QString &systemLocation)
{
#ifdef Q_OS_LINUX
    const mode_t flags = O_RDWR | O_NONBLOCK | O_NOCTTY;
    const int fd = qt_safe_open(systemLocation.toLocal8Bit().constData(), flags);
    if (fd != -1) {
        struct serial_struct serinfo;
        const int retval = ::ioctl(fd, TIOCGSERIAL, &serinfo);
        qt_safe_close(fd);
        if (retval != -1 && serinfo.type != PORT_UNKNOWN)
            return true;
    }
#else
    Q_UNUSED(systemLocation);
#endif
    return false;
}

static bool isRfcommDevice(QStringView portName)
{
    if (!portName.startsWith(QLatin1String("rfcomm")))
        return false;

    bool ok;
    const int portNumber = portName.mid(6).toInt(&ok);
    if (!ok || (portNumber < 0) || (portNumber > 255))
        return false;
    return true;
}

// provided by the tty0tty driver
static bool isVirtualNullModemDevice(const QString &portName)
{
    return portName.startsWith(QLatin1String("tnt"));
}

// provided by the g_serial driver
static bool isGadgetDevice(const QString &portName)
{
    return portName.startsWith(QLatin1String("ttyGS"));
}

static QString ueventProperty(const QDir &targetDir, const QByteArray &pattern)
{
    QFile f(QFileInfo(targetDir, QStringLiteral("uevent")).absoluteFilePath());
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();

    const QByteArray content = f.readAll();

    const qsizetype firstbound = content.indexOf(pattern);
    if (firstbound == -1)
        return QString();

    qsizetype lastbound = content.indexOf('\n', firstbound);
    if (lastbound == -1)
        lastbound = content.size();
    return QString::fromLatin1(
                content.mid(firstbound + pattern.size(),
                            lastbound - firstbound - pattern.size()))
            .simplified();
}

static QString deviceName(const QDir &targetDir)
{
    return ueventProperty(targetDir, "DEVNAME=");
}

static QString deviceDriver(const QDir &targetDir)
{
    const QDir deviceDir(targetDir.absolutePath() + QLatin1String("/device"));
    return ueventProperty(deviceDir, "DRIVER=");
}

static QString deviceProperty(const QString &targetFilePath)
{
    QFile f(targetFilePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();
   return QString::fromLatin1(f.readAll()).simplified();
}

static QString deviceDescription(const QDir &targetDir)
{
    return deviceProperty(QFileInfo(targetDir, QStringLiteral("product")).absoluteFilePath());
}

static QString deviceManufacturer(const QDir &targetDir)
{
    return deviceProperty(QFileInfo(targetDir, QStringLiteral("manufacturer")).absoluteFilePath());
}

static quint16 deviceProductIdentifier(const QDir &targetDir, bool &hasIdentifier)
{
    QString result = deviceProperty(QFileInfo(targetDir, QStringLiteral("idProduct")).absoluteFilePath());
    if (result.isEmpty())
        result = deviceProperty(QFileInfo(targetDir, QStringLiteral("device")).absoluteFilePath());
    return result.toInt(&hasIdentifier, 16);
}

static quint16 deviceVendorIdentifier(const QDir &targetDir, bool &hasIdentifier)
{
    QString result = deviceProperty(QFileInfo(targetDir, QStringLiteral("idVendor")).absoluteFilePath());
    if (result.isEmpty())
        result = deviceProperty(QFileInfo(targetDir, QStringLiteral("vendor")).absoluteFilePath());
    return result.toInt(&hasIdentifier, 16);
}

static QString deviceSerialNumber(const QDir &targetDir)
{
    return deviceProperty(QFileInfo(targetDir, QStringLiteral("serial")).absoluteFilePath());
}

QList<QSerialPortInfo> availablePortsBySysfs(bool &ok)
{
    QDir ttySysClassDir(QStringLiteral("/sys/class/tty"));

    if (!(ttySysClassDir.exists() && ttySysClassDir.isReadable())) {
        ok = false;
        return QList<QSerialPortInfo>();
    }

    QList<QSerialPortInfo> serialPortInfoList;
    ttySysClassDir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
    const auto fileInfos = ttySysClassDir.entryInfoList();
    for (const QFileInfo &fileInfo : fileInfos) {
        if (!fileInfo.isSymLink())
            continue;

        QDir targetDir(fileInfo.symLinkTarget());

        QSerialPortInfoPrivate priv;

        priv.portName = deviceName(targetDir);
        if (priv.portName.isEmpty())
            continue;

        const QString driverName = deviceDriver(targetDir);
        if (driverName.isEmpty()) {
            if (!isRfcommDevice(priv.portName)
                    && !isVirtualNullModemDevice(priv.portName)
                    && !isGadgetDevice(priv.portName)) {
                continue;
            }
        }

        priv.device = QSerialPortInfoPrivate::portNameToSystemLocation(priv.portName);
        if (isSerial8250Driver(driverName) && !isValidSerial8250(priv.device))
            continue;

        do {
            if (priv.description.isEmpty())
                priv.description = deviceDescription(targetDir);

            if (priv.manufacturer.isEmpty())
                priv.manufacturer = deviceManufacturer(targetDir);

            if (priv.serialNumber.isEmpty())
                priv.serialNumber = deviceSerialNumber(targetDir);

            if (!priv.hasVendorIdentifier)
                priv.vendorIdentifier = deviceVendorIdentifier(targetDir, priv.hasVendorIdentifier);

            if (!priv.hasProductIdentifier)
                priv.productIdentifier = deviceProductIdentifier(targetDir, priv.hasProductIdentifier);

            if (!priv.description.isEmpty()
                    || !priv.manufacturer.isEmpty()
                    || !priv.serialNumber.isEmpty()
                    || priv.hasVendorIdentifier
                    || priv.hasProductIdentifier) {
                break;
            }
        } while (targetDir.cdUp());

        serialPortInfoList.append(priv);
    }

    ok = true;
    return serialPortInfoList;
}

struct udev_deleter {
    void operator()(struct ::udev *pointer) const
    {
        ::udev_unref(pointer);
    }
    void operator()(struct ::udev_enumerate *pointer) const
    {
        ::udev_enumerate_unref(pointer);
    }
    void operator()(struct ::udev_device *pointer) const
    {
        ::udev_device_unref(pointer);
    }
};
template <typename T>
using udev_ptr = std::unique_ptr<T, udev_deleter>;

#ifndef LINK_LIBUDEV
    Q_GLOBAL_STATIC(QLibrary, udevLibrary)
#endif

static QString deviceProperty(struct ::udev_device *dev, const char *name)
{
    return QString::fromLatin1(::udev_device_get_property_value(dev, name));
}

static QString deviceDriver(struct ::udev_device *dev)
{
    return QString::fromLatin1(::udev_device_get_driver(dev));
}

static QString deviceDescription(struct ::udev_device *dev)
{
    return deviceProperty(dev, "ID_MODEL").replace(QLatin1Char('_'), QLatin1Char(' '));
}

static QString deviceManufacturer(struct ::udev_device *dev)
{
    return deviceProperty(dev, "ID_VENDOR").replace(QLatin1Char('_'), QLatin1Char(' '));
}

static quint16 deviceProductIdentifier(struct ::udev_device *dev, bool &hasIdentifier)
{
    return deviceProperty(dev, "ID_MODEL_ID").toInt(&hasIdentifier, 16);
}

static quint16 deviceVendorIdentifier(struct ::udev_device *dev, bool &hasIdentifier)
{
    return deviceProperty(dev, "ID_VENDOR_ID").toInt(&hasIdentifier, 16);
}

static QString deviceSerialNumber(struct ::udev_device *dev)
{
    return deviceProperty(dev,"ID_SERIAL_SHORT");
}

static QString deviceName(struct ::udev_device *dev)
{
    return QString::fromLatin1(::udev_device_get_sysname(dev));
}

static QString deviceLocation(struct ::udev_device *dev)
{
    return QString::fromLatin1(::udev_device_get_devnode(dev));
}

QList<QSerialPortInfo> availablePortsByUdev(bool &ok)
{
    ok = false;

#ifndef LINK_LIBUDEV
    static bool symbolsResolved = resolveSymbols(udevLibrary());
    if (!symbolsResolved)
        return QList<QSerialPortInfo>();
#endif

    const udev_ptr<struct ::udev> udev(::udev_new());

    if (!udev)
        return QList<QSerialPortInfo>();

    const udev_ptr<udev_enumerate> enumerate(::udev_enumerate_new(udev.get()));

    if (!enumerate)
        return QList<QSerialPortInfo>();

    ::udev_enumerate_add_match_subsystem(enumerate.get(), "tty");
    ::udev_enumerate_scan_devices(enumerate.get());

    udev_list_entry *devices = ::udev_enumerate_get_list_entry(enumerate.get());

    QList<QSerialPortInfo> serialPortInfoList;
    udev_list_entry *dev_list_entry;
    udev_list_entry_foreach(dev_list_entry, devices) {

        ok = true;

        const udev_ptr<udev_device>
                dev(::udev_device_new_from_syspath(
                        udev.get(), ::udev_list_entry_get_name(dev_list_entry)));

        if (!dev)
            continue;

        QSerialPortInfoPrivate priv;

        priv.device = deviceLocation(dev.get());
        priv.portName = deviceName(dev.get());

        udev_device *parentdev = ::udev_device_get_parent(dev.get());

        if (parentdev) {
            const QString driverName = deviceDriver(parentdev);
            if (isSerial8250Driver(driverName) && !isValidSerial8250(priv.device))
                continue;
            priv.description = deviceDescription(dev.get());
            priv.manufacturer = deviceManufacturer(dev.get());
            priv.serialNumber = deviceSerialNumber(dev.get());
            priv.vendorIdentifier = deviceVendorIdentifier(dev.get(), priv.hasVendorIdentifier);
            priv.productIdentifier = deviceProductIdentifier(dev.get(), priv.hasProductIdentifier);
        } else {
            if (!isRfcommDevice(priv.portName)
                    && !isVirtualNullModemDevice(priv.portName)
                    && !isGadgetDevice(priv.portName)) {
                continue;
            }
        }

        serialPortInfoList.append(priv);
    }

    return serialPortInfoList;
}

QList<QSerialPortInfo> QSerialPortInfo::availablePorts()
{
    bool ok = false;
    QList<QSerialPortInfo> serialPortInfoList;

    const bool skipUdevLookup = qEnvironmentVariableIsSet(SkipUdevEnvVarName);
    if (!skipUdevLookup)
        serialPortInfoList = availablePortsByUdev(ok);

#ifdef Q_OS_LINUX
    if (!ok)
        serialPortInfoList = availablePortsBySysfs(ok);
#endif

    if (!ok)
        serialPortInfoList = availablePortsByFiltersOfDevices(ok);

    return serialPortInfoList;
}

QString QSerialPortInfoPrivate::portNameToSystemLocation(const QString &source)
{
    return (source.startsWith(QLatin1Char('/'))
            || source.startsWith(QLatin1String("./"))
            || source.startsWith(QLatin1String("../")))
            ? source : (QLatin1String("/dev/") + source);
}

QString QSerialPortInfoPrivate::portNameFromSystemLocation(const QString &source)
{
    return source.startsWith(QLatin1String("/dev/"))
            ? source.mid(5) : source;
}

QT_END_NAMESPACE
