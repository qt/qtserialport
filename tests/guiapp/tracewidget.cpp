#include <QtGui/QScrollBar>

#include "tracewidget.h"
#include "ui_tracewidget.h"

#include "serialport.h"


/* Public methods */


TraceWidget::TraceWidget(SerialPort *port, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TraceWidget)
    , m_port(port)
{
    ui->setupUi(this);
    ui->textEdit->document()->setMaximumBlockCount(100);

    connect(ui->sendButton, SIGNAL(clicked()), this, SLOT(procSendButtonClick()));
    connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(procClearButtonClick()));

    connect(m_port, SIGNAL(readyRead()), this, SLOT(procReadyRead()));
}

TraceWidget::~TraceWidget()
{
    delete ui;
}


/* Protected methods */


void TraceWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}


/* Private slots */


void TraceWidget::printTrace(const QByteArray &data, bool directionRx)
{
    ui->textEdit->setTextColor((directionRx) ? Qt::darkBlue : Qt::darkGreen);
    ui->textEdit->insertPlainText(QString(data));

    QScrollBar *bar = ui->textEdit->verticalScrollBar();
    bar->setValue(bar->maximum());
}

void TraceWidget::procSendButtonClick()
{
    QByteArray data;
    data.append(ui->lineEdit->text());
    if (data.size() > 0) {
        m_port->write(data);
        printTrace(data, false);
    }
}

void TraceWidget::procClearButtonClick()
{
    ui->textEdit->clear();
}

void TraceWidget::procReadyRead()
{
    QByteArray data = m_port->readAll();
    printTrace(data, true);
}
