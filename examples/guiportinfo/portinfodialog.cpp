#include "portinfodialog.h"
#include "ui_portinfodialog.h"

#include "serialportinfo.h"

#include <QtCore/QVariant>

Q_DECLARE_METATYPE(SerialPortInfo)

Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
{
    ui->setupUi(this);
    procUpdateAvailablePorts();
    procItemPortChanged(0);

    connect(ui->updateButton, SIGNAL(clicked()), this, SLOT(procUpdateAvailablePorts()));
    connect(ui->portsComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(procItemPortChanged(int)));
    connect(ui->busyButton, SIGNAL(clicked()), this, SLOT(procBusyButtonClick()));
    connect(ui->validButton, SIGNAL(clicked()), this, SLOT(procValidButtonClick()));
    connect(ui->ratesButton, SIGNAL(clicked()), this, SLOT(procRatesButtonClick()));
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::procUpdateAvailablePorts()
{
    ui->portsComboBox->clear();
    foreach (SerialPortInfo info, SerialPortInfo::availablePorts()) {
        QVariant v;
        v.setValue(info);
        ui->portsComboBox->addItem(info.portName(), v);
    }
}

void Dialog::procItemPortChanged(int idx)
{
    QVariant v = ui->portsComboBox->itemData(idx);
    if (v.isValid()) {
        SerialPortInfo info = v.value<SerialPortInfo>();

        ui->locationValueLabel->setText(info.systemLocation());
        ui->descriptionValueLabel->setText(info.description());
        ui->manufacturerValueLabel->setText(info.manufacturer());

        ui->busyLabel->setText("***");
        ui->validLabel->setText("***");
        ui->ratesComboBox->clear();
    }
}

void Dialog::procBusyButtonClick()
{
    int idx = ui->portsComboBox->currentIndex();
    if (idx >= 0) {
        QVariant v = ui->portsComboBox->itemData(idx);
        if (v.isValid()) {
            SerialPortInfo info = v.value<SerialPortInfo>();
            ui->busyLabel->setText(info.isBusy() ? tr("Busy") : tr("Free"));
        }
    }
}

void Dialog::procValidButtonClick()
{
    int idx = ui->portsComboBox->currentIndex();
    if (idx >= 0) {
        QVariant v = ui->portsComboBox->itemData(idx);
        if (v.isValid()) {
            SerialPortInfo info = v.value<SerialPortInfo>();
            ui->validLabel->setText(info.isValid() ? tr("Valid") : tr("Invalid"));
        }
    }
}

void Dialog::procRatesButtonClick()
{
    ui->ratesComboBox->clear();
    int idx = ui->portsComboBox->currentIndex();
    if (idx >= 0) {
        QVariant v = ui->portsComboBox->itemData(idx);
        if (v.isValid()) {
            SerialPortInfo info = v.value<SerialPortInfo>();

            foreach (qint32 rate, info.standardRates())
                ui->ratesComboBox->addItem(QString::number(rate));
        }
    }
}
