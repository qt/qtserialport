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
    m_serialPortLabel(new QLabel(tr("Serial port:"))),
    m_serialPortComboBox(new QComboBox),
    m_waitRequestLabel(new QLabel(tr("Wait request, msec:"))),
    m_waitRequestSpinBox(new QSpinBox),
    m_responseLabel(new QLabel(tr("Response:"))),
    m_responseLineEdit(new QLineEdit(tr("Hello, I'm the receiver."))),
    m_trafficLabel(new QLabel(tr("No traffic."))),
    m_statusLabel(new QLabel(tr("Status: Not running."))),
    m_runButton(new QPushButton(tr("Start")))
{
    m_waitRequestSpinBox->setRange(0, 10000);
    m_waitRequestSpinBox->setValue(10000);

    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos)
        m_serialPortComboBox->addItem(info.portName());

    auto mainLayout = new QGridLayout;
    mainLayout->addWidget(m_serialPortLabel, 0, 0);
    mainLayout->addWidget(m_serialPortComboBox, 0, 1);
    mainLayout->addWidget(m_waitRequestLabel, 1, 0);
    mainLayout->addWidget(m_waitRequestSpinBox, 1, 1);
    mainLayout->addWidget(m_runButton, 0, 2, 2, 1);
    mainLayout->addWidget(m_responseLabel, 2, 0);
    mainLayout->addWidget(m_responseLineEdit, 2, 1, 1, 3);
    mainLayout->addWidget(m_trafficLabel, 3, 0, 1, 4);
    mainLayout->addWidget(m_statusLabel, 4, 0, 1, 5);
    setLayout(mainLayout);

    setWindowTitle(tr("Blocking Receiver"));
    m_serialPortComboBox->setFocus();

    connect(m_runButton, &QPushButton::clicked, this, &Dialog::startReceiver);
    connect(&m_thread, &ReceiverThread::request, this,&Dialog::showRequest);
    connect(&m_thread, &ReceiverThread::error, this, &Dialog::processError);
    connect(&m_thread, &ReceiverThread::timeout, this, &Dialog::processTimeout);

    connect(m_serialPortComboBox, &QComboBox::currentIndexChanged, this, &Dialog::activateRunButton);
    connect(m_waitRequestSpinBox, &QSpinBox::textChanged, this, &Dialog::activateRunButton);
    connect(m_responseLineEdit, &QLineEdit::textChanged, this, &Dialog::activateRunButton);
}

void Dialog::startReceiver()
{
    m_runButton->setEnabled(false);
    m_statusLabel->setText(tr("Status: Running, connected to port %1.")
                           .arg(m_serialPortComboBox->currentText()));
    m_thread.startReceiver(m_serialPortComboBox->currentText(),
                        m_waitRequestSpinBox->value(),
                        m_responseLineEdit->text());
}

void Dialog::showRequest(const QString &s)
{
    m_trafficLabel->setText(tr("Traffic, transaction #%1:"
                               "\n\r-request: %2"
                               "\n\r-response: %3")
                            .arg(++m_transactionCount)
                            .arg(s)
                            .arg(m_responseLineEdit->text()));
}

void Dialog::processError(const QString &s)
{
    activateRunButton();
    m_statusLabel->setText(tr("Status: Not running, %1.").arg(s));
    m_trafficLabel->setText(tr("No traffic."));
}

void Dialog::processTimeout(const QString &s)
{
    m_statusLabel->setText(tr("Status: Running, %1.").arg(s));
    m_trafficLabel->setText(tr("No traffic."));
}

void Dialog::activateRunButton()
{
    m_runButton->setEnabled(true);
}
