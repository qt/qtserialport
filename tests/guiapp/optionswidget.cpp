#include <QtGui/QMessageBox>

#include "optionswidget.h"
#include "ui_optionswidget.h"

#include "serialport.h"


/* Public methods */


OptionsWidget::OptionsWidget(SerialPort *port, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::OptionsWidget)
    , m_port(port)
    , m_rate(0), m_data(0), m_parity(0), m_stop(0), m_flow(0), m_policy(0)
{
    ui->setupUi(this);
    procFillingOptions();

    connect(ui->applyButton, SIGNAL(clicked()), this, SLOT(procApplyButtonClick()));
}

OptionsWidget::~OptionsWidget()
{
    delete ui;
}


/* Protected methods. */


void OptionsWidget::showEvent(QShowEvent *e)
{
    Q_UNUSED(e)
    detectOptions();
}


/* Private slots */


void OptionsWidget::procFillingOptions()
{
    ui->baudBox->addItem(tr("9600"), SerialPort::Rate9600);
    ui->baudBox->addItem(tr("19200"), SerialPort::Rate19200);
    ui->baudBox->addItem(tr("38400"), SerialPort::Rate38400);
    ui->baudBox->addItem(tr("57600"), SerialPort::Rate57600);
    ui->baudBox->addItem(tr("115200"), SerialPort::Rate115200);
    ui->baudBox->addItem(tr("Unknown"), SerialPort::UnknownRate);

    ui->dataBox->addItem(tr("5"), SerialPort::Data5);
    ui->dataBox->addItem(tr("6"), SerialPort::Data6);
    ui->dataBox->addItem(tr("7"), SerialPort::Data7);
    ui->dataBox->addItem(tr("8"), SerialPort::Data8);
    ui->dataBox->addItem(tr("Unknown"), SerialPort::UnknownDataBits);

    ui->parityBox->addItem(tr("None"), SerialPort::NoParity);
    ui->parityBox->addItem(tr("Even"), SerialPort::EvenParity);
    ui->parityBox->addItem(tr("Odd"), SerialPort::OddParity);
    ui->parityBox->addItem(tr("Mark"), SerialPort::MarkParity);
    ui->parityBox->addItem(tr("Space"), SerialPort::SpaceParity);
    ui->parityBox->addItem(tr("Unknown"), SerialPort::UnknownParity);

    ui->stopBox->addItem(tr("1"), SerialPort::OneStop);
    ui->stopBox->addItem(tr("1.5"), SerialPort::OneAndHalfStop);
    ui->stopBox->addItem(tr("2"), SerialPort::TwoStop);
    ui->stopBox->addItem(tr("Unknown"), SerialPort::UnknownStopBits);

    ui->flowBox->addItem(tr("Off"), SerialPort::NoFlowControl);
    ui->flowBox->addItem(tr("Hardware"), SerialPort::HardwareControl);
    ui->flowBox->addItem(tr("Software"), SerialPort::SoftwareControl);
    ui->flowBox->addItem(tr("Unknown"), SerialPort::UnknownFlowControl);

    ui->policyBox->addItem(tr("Skip"), SerialPort::SkipPolicy);
    ui->policyBox->addItem(tr("PassZero"), SerialPort::PassZeroPolicy);
    ui->policyBox->addItem(tr("Ignore"), SerialPort::IgnorePolicy);
    ui->policyBox->addItem(tr("StopReceiving"), SerialPort::StopReceivingPolicy);
    ui->policyBox->addItem(tr("Unknown"), SerialPort::UnknownPolicy);
}

void OptionsWidget::procApplyButtonClick()
{
    bool ok;
    bool hasChanged = false;

    int val = ui->baudBox->itemData(ui->baudBox->currentIndex()).toInt(&ok);
    if (val != m_rate) {
        m_port->setRate(SerialPort::Rate(val));
        hasChanged = true;
    }

    val = ui->dataBox->itemData(ui->dataBox->currentIndex()).toInt(&ok);
    if (val != m_data) {
        m_port->setDataBits(SerialPort::DataBits(val));
        hasChanged = true;
    }

    val = ui->parityBox->itemData(ui->parityBox->currentIndex()).toInt(&ok);
    if (val != m_parity) {
        m_port->setParity(SerialPort::Parity(val));
        hasChanged = true;
    }

    val = ui->stopBox->itemData(ui->stopBox->currentIndex()).toInt(&ok);
    if (val != m_stop) {
        m_port->setStopBits(SerialPort::StopBits(val));
        hasChanged = true;
    }

    val = ui->flowBox->itemData(ui->flowBox->currentIndex()).toInt(&ok);
    if (val != m_flow) {
        m_port->setFlowControl(SerialPort::FlowControl(val));
        hasChanged = true;
    }

    val = ui->policyBox->itemData(ui->policyBox->currentIndex()).toInt(&ok);
    if (val != m_policy) {
        m_port->setDataErrorPolicy(SerialPort::DataErrorPolicy(val));
        hasChanged = true;
    }

    if (hasChanged)
        detectOptions();
}


/* Private methods */


void OptionsWidget::detectOptions()
{
    m_rate = m_port->rate();
    switch (m_rate) {
    case SerialPort::Rate9600:
    case SerialPort::Rate19200:
    case SerialPort::Rate38400:
    case SerialPort::Rate57600:
    case SerialPort::Rate115200:
        break;
    default: m_rate = SerialPort::UnknownRate;
    }
    int count = ui->baudBox->count();
    for (int i = 0; i < count; ++i) {
        bool ok;
        if (ui->baudBox->itemData(i).toInt(&ok) == m_rate) {
            ui->baudBox->setCurrentIndex(i);
            break;
        }
    }

    m_data = m_port->dataBits();
    switch (m_data) {
    case SerialPort::Data5:
    case SerialPort::Data6:
    case SerialPort::Data7:
    case SerialPort::Data8:
        break;
    default: m_data = SerialPort::UnknownDataBits;
    }
    count = ui->dataBox->count();
    for (int i = 0; i < count; ++i) {
        bool ok;
        if (ui->dataBox->itemData(i).toInt(&ok) == m_data) {
            ui->dataBox->setCurrentIndex(i);
            break;
        }
    }

    m_parity = m_port->parity();
    switch (m_parity) {
    case SerialPort::NoParity:
    case SerialPort::EvenParity:
    case SerialPort::OddParity:
    case SerialPort::MarkParity:
    case SerialPort::SpaceParity:
        break;
    default: m_parity = SerialPort::UnknownParity;
    }
    count = ui->parityBox->count();
    for (int i = 0; i < count; ++i) {
        bool ok;
        if (ui->parityBox->itemData(i).toInt(&ok) == m_parity) {
            ui->parityBox->setCurrentIndex(i);
            break;
        }
    }

    m_stop = m_port->stopBits();
    switch (m_stop) {
    case SerialPort::OneStop:
    case SerialPort::OneAndHalfStop:
    case SerialPort::TwoStop:
        break;
    default: m_stop = SerialPort::UnknownStopBits;
    }
    count = ui->stopBox->count();
    for (int i = 0; i < count; ++i) {
        bool ok;
        if (ui->stopBox->itemData(i).toInt(&ok) == m_stop) {
            ui->stopBox->setCurrentIndex(i);
            break;
        }
    }

    m_flow = m_port->flowControl();
    switch (m_flow) {
    case SerialPort::NoFlowControl:
    case SerialPort::HardwareControl:
    case SerialPort::SoftwareControl:
        break;
    default: m_flow = SerialPort::UnknownFlowControl;
    }
    count = ui->flowBox->count();
    for (int i = 0; i < count; ++i) {
        bool ok;
        if (ui->flowBox->itemData(i).toInt(&ok) == m_flow) {
            ui->flowBox->setCurrentIndex(i);
            break;
        }
    }

    m_policy = m_port->dataErrorPolicy();
    switch (m_policy) {
    case SerialPort::PassZeroPolicy:
    case SerialPort::IgnorePolicy:
    case SerialPort::StopReceivingPolicy:
        break;
    default: m_flow = SerialPort::UnknownPolicy;
    }
    count = ui->policyBox->count();
    for (int i = 0; i < count; ++i) {
        bool ok;
        if (ui->policyBox->itemData(i).toInt(&ok) == m_policy) {
            ui->policyBox->setCurrentIndex(i);
            break;
        }
    }
}










