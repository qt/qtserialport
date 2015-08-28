/****************************************************************************
**
** Copyright (C) 2011-2012 Denis Shienkov <denis.shienkov@gmail.com>
** Copyright (C) 2011 Sergey Belyashov <Sergey.Belyashov@gmail.com>
** Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtSerialPort module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qserialportinfo.h"
#include "qserialportinfo_p.h"
#include "qserialport_p.h"

#include <QtCore/qlockfile.h>
#include <QtCore/qfile.h>
#include <QtCore/qdir.h>
#include <QtCore/qscopedpointer.h>

#ifdef Q_OS_FREEBSD
#include <QtCore/qdatastream.h>
#include <QtCore/qvector.h>
#endif

#include <private/qcore_unix_p.h>

#include <errno.h>
#include <sys/types.h> // kill
#include <signal.h>    // kill

#ifdef Q_OS_FREEBSD
#include <sys/sysctl.h> // sysctl, sysctlnametomib
#endif

#include "qtudev_p.h"

QT_BEGIN_NAMESPACE

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
        foreach (const QFileInfo &deviceFileInfo, deviceDir.entryInfoList()) {
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

    foreach (const QString &deviceFilePath, filteredDeviceFilePaths()) {
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

#ifdef Q_OS_FREEBSD

static QString deviceProperty(const QString &pnpinfo, const QByteArray &pattern)
{
    const int firstbound = pnpinfo.indexOf(QLatin1String(pattern));
    if (firstbound == -1)
        return QString();
    const int lastbound = pnpinfo.indexOf(QLatin1Char(' '), firstbound);
    return pnpinfo.mid(firstbound + pattern.size(), lastbound - firstbound - pattern.size());
}

static QString deviceName(const QString &pnpinfo)
{
    return deviceProperty(pnpinfo, "ttyname=");
}

static QString deviceCount(const QString &pnpinfo)
{
    return deviceProperty(pnpinfo, "ttyports=");
}

static quint16 deviceProductIdentifier(const QString &pnpinfo, bool &hasIdentifier)
{
    QString result = deviceProperty(pnpinfo, "product=");
    return result.toInt(&hasIdentifier, 16);
}

static quint16 deviceVendorIdentifier(const QString &pnpinfo, bool &hasIdentifier)
{
    QString result = deviceProperty(pnpinfo, "vendor=");
    return result.toInt(&hasIdentifier, 16);
}

static QString deviceSerialNumber(const QString &pnpinfo)
{
    QString serialNumber = deviceProperty(pnpinfo, "sernum=");
    serialNumber.remove(QLatin1Char('"'));
    return serialNumber;
}

// A 'desc' string contains the both description and manufacturer
// properties, which are not possible to extract from the source
// string. Besides, this string can contains an other information,
// which should be excluded from the result.
static QString deviceDescriptionAndManufacturer(const QString &desc)
{
    const int classindex = desc.indexOf(QLatin1String(", class "));
    if (classindex == -1)
        return desc;
    return desc.mid(0, classindex);
}

struct NodeInfo
{
    QString name;
    QString value;
};

static QVector<int> mibFromName(const QString &name)
{
    size_t mibsize = 0;
    if (::sysctlnametomib(name.toLocal8Bit().constData(), Q_NULLPTR, &mibsize) < 0
            || mibsize == 0) {
        return QVector<int>();
    }
    QVector<int> mib(mibsize);
    if (::sysctlnametomib(name.toLocal8Bit().constData(), &mib[0], &mibsize) < 0)
        return QVector<int>();

    return mib;
}

static QVector<int> nextOid(const QVector<int> &previousOid)
{
    QVector<int> mib;
    mib.append(0); // Magic undocumented code (CTL_UNSPEC ?)
    mib.append(2); // Magic undocumented code
    foreach (int code, previousOid)
        mib.append(code);

    size_t requiredLength = 0;
    if (::sysctl(&mib[0], mib.count(), Q_NULLPTR, &requiredLength, Q_NULLPTR, 0) < 0)
        return QVector<int>();
    const size_t oidLength = requiredLength / sizeof(int);
    QVector<int> oid(oidLength, 0);
    if (::sysctl(&mib[0], mib.count(), &oid[0], &requiredLength, Q_NULLPTR, 0) < 0)
        return QVector<int>();

    if (previousOid.first() != oid.first())
        return QVector<int>();

    return oid;
}

static NodeInfo nodeForOid(const QVector<int> &oid)
{
    QVector<int> mib;
    mib.append(0); // Magic undocumented code (CTL_UNSPEC ?)
    mib.append(1); // Magic undocumented code
    foreach (int code, oid)
        mib.append(code);

    // query node name
    size_t requiredLength = 0;
    if (::sysctl(&mib[0], mib.count(), Q_NULLPTR, &requiredLength, Q_NULLPTR, 0) < 0)
        return NodeInfo();
    QByteArray name(requiredLength, 0);
    if (::sysctl(&mib[0], mib.count(), name.data(), &requiredLength, Q_NULLPTR, 0) < 0)
        return NodeInfo();

    // query node value
    requiredLength = 0;
    if (::sysctl(&oid[0], oid.count(), Q_NULLPTR, &requiredLength, Q_NULLPTR, 0) < 0)
        return NodeInfo();
    QByteArray value(requiredLength, 0);
    if (::sysctl(&oid[0], oid.count(), value.data(), &requiredLength, Q_NULLPTR, 0) < 0)
        return NodeInfo();

    // query value format
    mib[1] = 4; // Magic undocumented code
    requiredLength = 0;
    if (::sysctl(&mib[0], mib.count(), Q_NULLPTR, &requiredLength, Q_NULLPTR, 0) < 0)
        return NodeInfo();
    QByteArray buf(requiredLength, 0);
    if (::sysctl(&mib[0], mib.count(), buf.data(), &requiredLength, Q_NULLPTR, 0) < 0)
        return NodeInfo();

    QDataStream in(buf);
    in.setByteOrder(QDataStream::LittleEndian);
    quint32 kind = 0;
    qint8 format = 0;
    in >> kind >> format;

    NodeInfo result;

    // we need only the string-type value
    if (format == 'A') {
        result.name = QString::fromLocal8Bit(name.constData());
        result.value = QString::fromLocal8Bit(value.constData());
    }

    return result;
}

static QList<NodeInfo> enumerateDesiredNodes(const QVector<int> &mib)
{
    QList<NodeInfo> nodes;

    QVector<int> oid = mib;

    forever {
        const QVector<int> nextoid = nextOid(oid);
        if (nextoid.isEmpty())
            break;

        const NodeInfo node = nodeForOid(nextoid);
        if (!node.name.isEmpty()) {
            if (node.name.endsWith("\%desc")
                    || node.name.endsWith("\%pnpinfo")) {
                nodes.append(node);
            }
        }

        oid = nextoid;
    }

    return nodes;
}

QList<QSerialPortInfo> availablePortsBySysctl(bool &ok)
{
    const QVector<int> mib = mibFromName(QLatin1String("dev"));
    if (mib.isEmpty()) {
        ok = false;
        return QList<QSerialPortInfo>();
    }

    const QList<NodeInfo> nodes = enumerateDesiredNodes(mib);
    if (nodes.isEmpty()) {
        ok = false;
        return QList<QSerialPortInfo>();
    }

    QDir deviceDir(QLatin1String("/dev"));
    if (!(deviceDir.exists() && deviceDir.isReadable())) {
        ok = false;
        return QList<QSerialPortInfo>();
    }

    deviceDir.setNameFilters(QStringList() << QLatin1String("cua*"));
    deviceDir.setFilter(QDir::Files | QDir::System | QDir::NoSymLinks);

    QList<QSerialPortInfo> serialPortInfoList;

    foreach (const QString &portName, deviceDir.entryList()) {
        if (portName.endsWith(QLatin1String(".init"))
                || portName.endsWith(QLatin1String(".lock"))) {
            continue;
        }

        QSerialPortInfoPrivate priv;
        priv.portName = portName;
        priv.device = QSerialPortInfoPrivate::portNameToSystemLocation(portName);

        foreach (const NodeInfo &node, nodes) {
            const int pnpinfoindex = node.name.indexOf(QLatin1String("\%pnpinfo"));
            if (pnpinfoindex == -1)
                continue;

            if (node.value.isEmpty())
                continue;

            QString ttyname = deviceName(node.value);
            if (ttyname.isEmpty())
                continue;

            const QString ttyportscount = deviceCount(node.value);
            if (ttyportscount.isEmpty())
                continue;

            const int count = ttyportscount.toInt();
            if (count == 0)
                continue;
            if (count > 1) {
                bool matched = false;
                for (int i = 0; i < count; ++i) {
                    const QString ends = QString(QLatin1String("%1.%2")).arg(ttyname).arg(i);
                    if (portName.endsWith(ends)) {
                        matched = true;
                        break;
                    }
                }

                if (!matched)
                    continue;
            } else {
                if (!portName.endsWith(ttyname))
                    continue;
            }

            priv.serialNumber = deviceSerialNumber(node.value);
            priv.vendorIdentifier = deviceVendorIdentifier(node.value, priv.hasVendorIdentifier);
            priv.productIdentifier = deviceProductIdentifier(node.value, priv.hasProductIdentifier);

            const QString nodebase = node.name.mid(0, pnpinfoindex);
            const QString descnode = QString(QLatin1String("%1\%desc")).arg(nodebase);

            // search for description and manufacturer properties
            foreach (const NodeInfo &node, nodes) {
                if (node.name != descnode)
                    continue;

                if (node.value.isEmpty())
                    continue;

                // We can not separate the description and the manufacturer
                // properties from the node value, so lets just duplicate it.
                priv.description = deviceDescriptionAndManufacturer(node.value);
                priv.manufacturer = priv.description;
                break;
            }

            break;
        }

        serialPortInfoList.append(priv);
    }

    ok = true;
    return serialPortInfoList;
}

#endif // Q_OS_FREEBSD

static bool isRfcommDevice(const QString &portName)
{
    if (!portName.startsWith(QLatin1String("rfcomm")))
        return false;

    bool ok;
    const int portNumber = portName.mid(6).toInt(&ok);
    if (!ok || (portNumber < 0) || (portNumber > 255))
        return false;
    return true;
}

static QString ueventProperty(const QDir &targetDir, const QByteArray &pattern)
{
    QFile f(QFileInfo(targetDir, QStringLiteral("uevent")).absoluteFilePath());
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();

    const QByteArray content = f.readAll();

    const int firstbound = content.indexOf(pattern);
    if (firstbound == -1)
        return QString();

    const int lastbound = content.indexOf('\n', firstbound);
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
    foreach (const QFileInfo &fileInfo, ttySysClassDir.entryInfoList()) {
        if (!fileInfo.isSymLink())
            continue;

        QDir targetDir(fileInfo.symLinkTarget());

        QSerialPortInfoPrivate priv;

        priv.portName = deviceName(targetDir);
        if (priv.portName.isEmpty())
            continue;

        const QString driverName = deviceDriver(targetDir);
        if (driverName.isEmpty()) {
            if (!isRfcommDevice(priv.portName))
                continue;
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

struct ScopedPointerUdevDeleter
{
    static inline void cleanup(struct ::udev *pointer)
    {
        ::udev_unref(pointer);
    }
};

struct ScopedPointerUdevEnumeratorDeleter
{
    static inline void cleanup(struct ::udev_enumerate *pointer)
    {
        ::udev_enumerate_unref(pointer);
    }
};

struct ScopedPointerUdevDeviceDeleter
{
    static inline void cleanup(struct ::udev_device *pointer)
    {
        ::udev_device_unref(pointer);
    }
};

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
    static const QString rfcommDeviceName(QStringLiteral("rfcomm"));

    QScopedPointer<struct ::udev, ScopedPointerUdevDeleter> udev(::udev_new());

    if (!udev)
        return QList<QSerialPortInfo>();

    QScopedPointer<udev_enumerate, ScopedPointerUdevEnumeratorDeleter>
            enumerate(::udev_enumerate_new(udev.data()));

    if (!enumerate)
        return QList<QSerialPortInfo>();

    ::udev_enumerate_add_match_subsystem(enumerate.data(), "tty");
    ::udev_enumerate_scan_devices(enumerate.data());

    udev_list_entry *devices = ::udev_enumerate_get_list_entry(enumerate.data());

    QList<QSerialPortInfo> serialPortInfoList;
    udev_list_entry *dev_list_entry;
    udev_list_entry_foreach(dev_list_entry, devices) {

        ok = true;

        QScopedPointer<udev_device, ScopedPointerUdevDeviceDeleter>
                dev(::udev_device_new_from_syspath(
                        udev.data(), ::udev_list_entry_get_name(dev_list_entry)));

        if (!dev)
            return serialPortInfoList;

        QSerialPortInfoPrivate priv;

        priv.device = deviceLocation(dev.data());
        priv.portName = deviceName(dev.data());

        udev_device *parentdev = ::udev_device_get_parent(dev.data());

        if (parentdev) {
            const QString driverName = deviceDriver(parentdev);
            if (isSerial8250Driver(driverName) && !isValidSerial8250(priv.portName))
                continue;
            priv.description = deviceDescription(dev.data());
            priv.manufacturer = deviceManufacturer(dev.data());
            priv.serialNumber = deviceSerialNumber(dev.data());
            priv.vendorIdentifier = deviceVendorIdentifier(dev.data(), priv.hasVendorIdentifier);
            priv.productIdentifier = deviceProductIdentifier(dev.data(), priv.hasProductIdentifier);
        } else {
            if (!isRfcommDevice(priv.portName))
                continue;
        }

        serialPortInfoList.append(priv);
    }

    return serialPortInfoList;
}

QList<QSerialPortInfo> QSerialPortInfo::availablePorts()
{
    bool ok;

    QList<QSerialPortInfo> serialPortInfoList = availablePortsByUdev(ok);

#ifdef Q_OS_LINUX
    if (!ok)
        serialPortInfoList = availablePortsBySysfs(ok);
#endif

#ifdef Q_OS_FREEBSD
    if (!ok)
        serialPortInfoList = availablePortsBySysctl(ok);
#endif

    if (!ok)
        serialPortInfoList = availablePortsByFiltersOfDevices(ok);

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

#if QT_DEPRECATED_SINCE(5, 2)
bool QSerialPortInfo::isValid() const
{
    QFile f(systemLocation());
    return f.exists();
}
#endif // QT_DEPRECATED_SINCE(5, 2)

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
