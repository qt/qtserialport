/****************************************************************************
**
** Copyright (C) 2012 Denis Shienkov <scapig@yandex.ru>
** Contact: http://www.qt-project.org/
**
** This file is part of the QtSerialPort module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "dialog.h"

#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QGridLayout>

#include <QtAddOnSerialPort/serialportinfo.h>

QT_USE_NAMESPACE_SERIALPORT

Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , transactionCount(0)
    , serialPortLabel(new QLabel(tr("Serial port:")))
    , serialPortComboBox(new QComboBox())
    , waitRequestLabel(new QLabel(tr("Wait request, msec:")))
    , waitRequestSpinBox(new QSpinBox())
    , responseLabel(new QLabel(tr("Response:")))
    , responseLineEdit(new QLineEdit(tr("Hello, I'm Slave.")))
    , trafficLabel(new QLabel(tr("No traffic.")))
    , statusLabel(new QLabel(tr("Status: Not running.")))
    , runButton(new QPushButton(tr("Start")))
{
    waitRequestSpinBox->setRange(0, 10000);
    waitRequestSpinBox->setValue(20);

    foreach (const SerialPortInfo &info, SerialPortInfo::availablePorts())
        serialPortComboBox->addItem(info.portName());

    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->addWidget(serialPortLabel, 0, 0);
    mainLayout->addWidget(serialPortComboBox, 0, 1);
    mainLayout->addWidget(waitRequestLabel, 1, 0);
    mainLayout->addWidget(waitRequestSpinBox, 1, 1);
    mainLayout->addWidget(runButton, 0, 2, 2, 1);
    mainLayout->addWidget(responseLabel, 2, 0);
    mainLayout->addWidget(responseLineEdit, 2, 1, 1, 3);
    mainLayout->addWidget(trafficLabel, 3, 0, 1, 4);
    mainLayout->addWidget(statusLabel, 4, 0, 1, 5);
    setLayout(mainLayout);

    setWindowTitle(tr("Slave"));
    serialPortComboBox->setFocus();

    timer.setSingleShot(true);

    connect(runButton, SIGNAL(clicked()),
            this, SLOT(startSlave()));
    connect(&serial, SIGNAL(readyRead()),
            this, SLOT(readRequest()));
    connect(&timer, SIGNAL(timeout()),
            this, SLOT(processTimeout()));

    connect(serialPortComboBox, SIGNAL(currentIndexChanged(QString)),
            this, SLOT(activateRunButton()));
    connect(waitRequestSpinBox, SIGNAL(valueChanged(int)),
            this, SLOT(activateRunButton()));
    connect(responseLineEdit, SIGNAL(textChanged(QString)),
            this, SLOT(activateRunButton()));
}

void Dialog::startSlave()
{
    if (serial.portName() != serialPortComboBox->currentText()) {
        serial.close();
        serial.setPort(serialPortComboBox->currentText());

        if (!serial.open(QIODevice::ReadWrite)) {
            processError(tr("Can't open %1, error code %2")
                         .arg(serial.portName()).arg(serial.error()));
            return;
        }

        if (!serial.setRate(9600)) {
            processError(tr("Can't set rate 9600 baud to port %1, error code %2")
                         .arg(serial.portName()).arg(serial.error()));
            return;
        }

        if (!serial.setDataBits(SerialPort::Data8)) {
            processError(tr("Can't set 8 data bits to port %1, error code %2")
                         .arg(serial.portName()).arg(serial.error()));
            return;
        }

        if (!serial.setParity(SerialPort::NoParity)) {
            processError(tr("Can't set no patity to port %1, error code %2")
                         .arg(serial.portName()).arg(serial.error()));
            return;
        }

        if (!serial.setStopBits(SerialPort::OneStop)) {
            processError(tr("Can't set 1 stop bit to port %1, error code %2")
                         .arg(serial.portName()).arg(serial.error()));
            return;
        }

        if (!serial.setFlowControl(SerialPort::NoFlowControl)) {
            processError(tr("Can't set no flow control to port %1, error code %2")
                         .arg(serial.portName()).arg(serial.error()));
            return;
        }
    }

    runButton->setEnabled(false);
    statusLabel->setText(tr("Status: Running, connected to port %1.")
                         .arg(serialPortComboBox->currentText()));
}

void Dialog::readRequest()
{
    if (!timer.isActive())
        timer.start(waitRequestSpinBox->value());
    request.append(serial.readAll());
}

void Dialog::processTimeout()
{
    serial.write(responseLineEdit->text().toLocal8Bit());

    trafficLabel->setText(tr("Traffic, transaction #%1:"
                             "\n\r-request: %2"
                             "\n\r-response: %3")
                          .arg(++transactionCount).arg(QString(request)).arg(responseLineEdit->text()));
    request.clear();
}

void Dialog::activateRunButton()
{
    runButton->setEnabled(true);
}

void Dialog::processError(const QString &s)
{
    activateRunButton();
    statusLabel->setText(tr("Status: Not running, %1.").arg(s));
    trafficLabel->setText(tr("No traffic."));
}
