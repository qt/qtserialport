#include <QtGui/QScrollBar>

#include "tracedialog.h"
#include "ui_tracedialog.h"

#include "serialport.h"


/* Public methods */


TraceDialog::TraceDialog(SerialPort *port, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::TraceDialog)
    , m_port(port)
{
    ui->setupUi(this);
    ui->textEdit->document()->setMaximumBlockCount(100);

    connect(ui->sendButton, SIGNAL(clicked()), this, SLOT(procSendButtonClick()));
    connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(procClearButtonClick()));

    connect(m_port, SIGNAL(readyRead()), this, SLOT(procReadyRead()));
}

TraceDialog::~TraceDialog()
{
    delete ui;
}


/* Protected methods */


void TraceDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}


/* Private slots */


void TraceDialog::printTrace(const QByteArray &data, bool directionRx)
{
    ui->textEdit->setTextColor((directionRx) ? Qt::darkBlue : Qt::darkGreen);
    ui->textEdit->insertPlainText(QString(data));

    QScrollBar *bar = ui->textEdit->verticalScrollBar();
    bar->setValue(bar->maximum());
}

void TraceDialog::procSendButtonClick()
{
    QByteArray data;
    data.append(ui->lineEdit->text());
    if (data.size() > 0) {
        m_port->write(data);
        printTrace(data, false);
    }
}

void TraceDialog::procClearButtonClick()
{
    ui->textEdit->clear();
}

void TraceDialog::procReadyRead()
{
    QByteArray data = m_port->readAll();
    printTrace(data, true);
}
