// Copyright (C) 2012 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "dialog.h"

#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSerialPortInfo>
#include <QSpinBox>

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    m_transactionCount(0),
    m_serialPortLabel(new QLabel(tr("Serial port:"))),
    m_serialPortComboBox(new QComboBox),
    m_waitResponseLabel(new QLabel(tr("Wait response, msec:"))),
    m_waitResponseSpinBox(new QSpinBox),
    m_requestLabel(new QLabel(tr("Request:"))),
    m_requestLineEdit(new QLineEdit(tr("Who are you?"))),
    m_trafficLabel(new QLabel(tr("No traffic."))),
    m_statusLabel(new QLabel(tr("Status: Not running."))),
    m_runButton(new QPushButton(tr("Start")))
{
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos)
        m_serialPortComboBox->addItem(info.portName());

    m_waitResponseSpinBox->setRange(0, 10000);
    m_waitResponseSpinBox->setValue(100);

    auto mainLayout = new QGridLayout;
    mainLayout->addWidget(m_serialPortLabel, 0, 0);
    mainLayout->addWidget(m_serialPortComboBox, 0, 1);
    mainLayout->addWidget(m_waitResponseLabel, 1, 0);
    mainLayout->addWidget(m_waitResponseSpinBox, 1, 1);
    mainLayout->addWidget(m_runButton, 0, 2, 2, 1);
    mainLayout->addWidget(m_requestLabel, 2, 0);
    mainLayout->addWidget(m_requestLineEdit, 2, 1, 1, 3);
    mainLayout->addWidget(m_trafficLabel, 3, 0, 1, 4);
    mainLayout->addWidget(m_statusLabel, 4, 0, 1, 5);
    setLayout(mainLayout);

    setWindowTitle(tr("Sender"));
    m_serialPortComboBox->setFocus();

    m_timer.setSingleShot(true);

    connect(m_runButton, &QPushButton::clicked, this, &Dialog::sendRequest);
    connect(&m_serial, &QSerialPort::readyRead, this, &Dialog::readResponse);
    connect(&m_timer, &QTimer::timeout, this, &Dialog::processTimeout);
}

void Dialog::sendRequest()
{
    if (m_serial.portName() != m_serialPortComboBox->currentText()) {
        m_serial.close();
        m_serial.setPortName(m_serialPortComboBox->currentText());

        if (!m_serial.open(QIODevice::ReadWrite)) {
            processError(tr("Can't open %1, error code %2")
                         .arg(m_serial.portName()).arg(m_serial.error()));
            return;
        }
    }

    setControlsEnabled(false);
    m_statusLabel->setText(tr("Status: Running, connected to port %1.")
                           .arg(m_serialPortComboBox->currentText()));

    m_serial.write(m_requestLineEdit->text().toUtf8());
    m_timer.start(m_waitResponseSpinBox->value());
}

void Dialog::readResponse()
{
    m_response.append(m_serial.readAll());
}

void Dialog::processTimeout()
{
    setControlsEnabled(true);
    m_trafficLabel->setText(tr("Traffic, transaction #%1:"
                               "\n\r-request: %2"
                               "\n\r-response: %3")
                            .arg(++m_transactionCount)
                            .arg(m_requestLineEdit->text())
                            .arg(QString::fromUtf8(m_response)));
    m_response.clear();
}

void Dialog::processError(const QString &error)
{
    setControlsEnabled(true);
    m_statusLabel->setText(tr("Status: Not running, %1.").arg(error));
    m_trafficLabel->setText(tr("No traffic."));
}

void Dialog::setControlsEnabled(bool enable)
{
    m_runButton->setEnabled(enable);
    m_serialPortComboBox->setEnabled(enable);
    m_waitResponseSpinBox->setEnabled(enable);
    m_requestLineEdit->setEnabled(enable);
}
