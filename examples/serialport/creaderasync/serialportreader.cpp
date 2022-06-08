// Copyright (C) 2013 Laszlo Papp <lpapp@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "serialportreader.h"

#include <QCoreApplication>

SerialPortReader::SerialPortReader(QSerialPort *serialPort, QObject *parent) :
    QObject(parent),
    m_serialPort(serialPort),
    m_standardOutput(stdout)
{
    connect(m_serialPort, &QSerialPort::readyRead, this, &SerialPortReader::handleReadyRead);
    connect(m_serialPort, &QSerialPort::errorOccurred, this, &SerialPortReader::handleError);
    connect(&m_timer, &QTimer::timeout, this, &SerialPortReader::handleTimeout);

    m_timer.start(5000);
}

void SerialPortReader::handleReadyRead()
{
    m_readData.append(m_serialPort->readAll());

    if (!m_timer.isActive())
        m_timer.start(5000);
}

void SerialPortReader::handleTimeout()
{
    if (m_readData.isEmpty()) {
        m_standardOutput << QObject::tr("No data was currently available "
                                        "for reading from port %1")
                            .arg(m_serialPort->portName())
                         << Qt::endl;
    } else {
        m_standardOutput << QObject::tr("Data successfully received from port %1")
                            .arg(m_serialPort->portName())
                         << Qt::endl;
        m_standardOutput << m_readData << Qt::endl;
    }

    QCoreApplication::quit();
}

void SerialPortReader::handleError(QSerialPort::SerialPortError serialPortError)
{
    if (serialPortError == QSerialPort::ReadError) {
        m_standardOutput << QObject::tr("An I/O error occurred while reading "
                                        "the data from port %1, error: %2")
                            .arg(m_serialPort->portName())
                            .arg(m_serialPort->errorString())
                         << Qt::endl;
        QCoreApplication::exit(1);
    }
}
