// Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QCoreApplication>
#include <QSerialPortInfo>
#include <QTextStream>

int main(int argc, char *argv[])
{
    QCoreApplication coreApplication(argc, argv);
    QTextStream out(stdout);
    const auto serialPortInfos = QSerialPortInfo::availablePorts();

    out << "Total number of ports available: " << serialPortInfos.count() << Qt::endl;

    const QStringView blankString = u"N/A";

    for (const QSerialPortInfo &serialPortInfo : serialPortInfos) {
        const QStringView description = serialPortInfo.description();
        const QStringView manufacturer = serialPortInfo.manufacturer();
        const QStringView serialNumber = serialPortInfo.serialNumber();

        out << Qt::endl
            << "Port: " << serialPortInfo.portName() << Qt::endl
            << "Location: " << serialPortInfo.systemLocation() << Qt::endl
            << "Description: " << (!description.isEmpty() ? description : blankString) << Qt::endl
            << "Manufacturer: " << (!manufacturer.isEmpty() ? manufacturer : blankString) << Qt::endl
            << "Serial number: " << (!serialNumber.isEmpty() ? serialNumber : blankString) << Qt::endl
            << "Vendor Identifier: " << (serialPortInfo.hasVendorIdentifier()
                                         ? QByteArray::number(serialPortInfo.vendorIdentifier(), 16)
                                         : blankString.toLatin1()) << Qt::endl
            << "Product Identifier: " << (serialPortInfo.hasProductIdentifier()
                                          ? QByteArray::number(serialPortInfo.productIdentifier(), 16)
                                          : blankString.toLatin1()) << Qt::endl;
    }

    return 0;
}
