#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include <QtAddOnSerialPort/serialportinfo.h>
#include <QIntValidator>
#include <QLineEdit>

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    intValidator = new QIntValidator(0, 4000000, this);

    ui->rateBox->setInsertPolicy(QComboBox::NoInsert);

    connect(ui->applyButton, SIGNAL(clicked()),
            this, SLOT(apply()));
    connect(ui->portsBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(showPortInfo(int)));
    connect(ui->rateBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(checkCustomRatePolicy(int)));

    fillPortsParameters();
    fillPortsInfo();

    updateSettings();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

SettingsDialog::Settings SettingsDialog::settings() const
{
    return currentSettings;
}

void SettingsDialog::showPortInfo(int idx)
{
    if (idx != -1) {
        QStringList list = ui->portsBox->itemData(idx).toStringList();
        ui->descriptionLabel->setText(tr("Description: %1").arg(list.at(1)));
        ui->manufacturerLabel->setText(tr("Manufacturer: %1").arg(list.at(2)));
        ui->locationLabel->setText(tr("Location: %1").arg(list.at(3)));
        ui->vidLabel->setText(tr("Vendor ID: %1").arg(list.at(4)));
        ui->pidLabel->setText(tr("Product ID: %1").arg(list.at(5)));
    }
}

void SettingsDialog::apply()
{
    updateSettings();
    hide();
}

void SettingsDialog::checkCustomRatePolicy(int idx)
{
    ui->rateBox->setEditable(idx == 4);
    if (idx == 4) {
        ui->rateBox->clearEditText();
        QLineEdit *edit = ui->rateBox->lineEdit();
        edit->setValidator(intValidator);
    }
}

void SettingsDialog::fillPortsParameters()
{
    // fill baud rate (is not the entire list of available values,
    // desired values??, add your independently)
    ui->rateBox->addItem(QLatin1String("9600"), 9600);
    ui->rateBox->addItem(QLatin1String("19200"), 19200);
    ui->rateBox->addItem(QLatin1String("38400"), 38400);
    ui->rateBox->addItem(QLatin1String("115200"), 115200);
    ui->rateBox->addItem(QLatin1String("Custom"));

    // fill data bits
    ui->dataBitsBox->addItem(QLatin1String("5"), static_cast<int>(SerialPort::Data5));
    ui->dataBitsBox->addItem(QLatin1String("6"), static_cast<int>(SerialPort::Data6));
    ui->dataBitsBox->addItem(QLatin1String("7"), static_cast<int>(SerialPort::Data7));
    ui->dataBitsBox->addItem(QLatin1String("8"), static_cast<int>(SerialPort::Data8));
    ui->dataBitsBox->setCurrentIndex(3);

    // fill parity
    ui->parityBox->addItem(QLatin1String("None"), static_cast<int>(SerialPort::NoParity));
    ui->parityBox->addItem(QLatin1String("Even"), static_cast<int>(SerialPort::EvenParity));
    ui->parityBox->addItem(QLatin1String("Odd"), static_cast<int>(SerialPort::OddParity));
    ui->parityBox->addItem(QLatin1String("Mark"), static_cast<int>(SerialPort::MarkParity));
    ui->parityBox->addItem(QLatin1String("Space"), static_cast<int>(SerialPort::SpaceParity));

    // fill stop bits
    ui->stopBitsBox->addItem(QLatin1String("1"), static_cast<int>(SerialPort::OneStop));
#if defined (Q_OS_WIN)
    ui->stopBitsBox->addItem(QLatin1String("1.5"), static_cast<int>(SerialPort::OneAndHalfStop));
#endif
    ui->stopBitsBox->addItem(QLatin1String("2"), static_cast<int>(SerialPort::TwoStop));

    // fill flow control
    ui->flowControlBox->addItem(QLatin1String("None"), static_cast<int>(SerialPort::NoFlowControl));
    ui->flowControlBox->addItem(QLatin1String("RTS/CTS"), static_cast<int>(SerialPort::HardwareControl));
    ui->flowControlBox->addItem(QLatin1String("XON/XOFF"), static_cast<int>(SerialPort::SoftwareControl));
}

void SettingsDialog::fillPortsInfo()
{
    ui->portsBox->clear();
    foreach (const SerialPortInfo &info, SerialPortInfo::availablePorts()) {
        QStringList list;
        list << info.portName() << info.description()
             << info.manufacturer() << info.systemLocation()
             << info.vendorIdentifier() << info.productIdentifier();

        ui->portsBox->addItem(list.at(0), list);
    }
}

void SettingsDialog::updateSettings()
{
    currentSettings.name = ui->portsBox->currentText();

    // Rate
    if (ui->rateBox->currentIndex() == 4) {
        // custom rate
        currentSettings.rate = ui->rateBox->currentText().toInt();
    } else {
        // standard rate
        currentSettings.rate = static_cast<SerialPort::Rate>(
                    ui->rateBox->itemData(ui->rateBox->currentIndex()).toInt());
    }
    currentSettings.stringRate = QString::number(currentSettings.rate);

    // Data bits
    currentSettings.dataBits = static_cast<SerialPort::DataBits>(
                ui->dataBitsBox->itemData(ui->dataBitsBox->currentIndex()).toInt());
    currentSettings.stringDataBits = ui->dataBitsBox->currentText();

    // Parity
    currentSettings.parity = static_cast<SerialPort::Parity>(
                ui->parityBox->itemData(ui->parityBox->currentIndex()).toInt());
    currentSettings.stringParity = ui->parityBox->currentText();

    // Stop bits
    currentSettings.stopBits = static_cast<SerialPort::StopBits>(
                ui->stopBitsBox->itemData(ui->stopBitsBox->currentIndex()).toInt());
    currentSettings.stringStopBits = ui->stopBitsBox->currentText();

    // Flow control
    currentSettings.flowControl = static_cast<SerialPort::FlowControl>(
                ui->flowControlBox->itemData(ui->flowControlBox->currentIndex()).toInt());
    currentSettings.stringFlowControl = ui->flowControlBox->currentText();
}
