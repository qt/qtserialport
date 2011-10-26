#include "unittests.h"
#include "serialport.h"

#include <QtCore/QTimer>
//#include <QtCore/QByteArray>


/* Public methods */

UnitTestSignals::UnitTestSignals(Logger *logger, QObject *parent)
    : UnitTestBase(UnitTestBase::SignalsUnitId, logger, parent)
    , m_started(false), m_transactionNum(0), m_bytesToWrite(0)
    , m_bytesReallyWrited(0), m_countSignalsBytesWritten(0)
    , m_countSignalsReadyRead(0)
{
    m_name = QString(tr("Signals Test"));
    m_description = QString(tr("\"Signals Test\" monitors and verifies the correctness of the\n"
                               "emission signals bytesWritten() and readyRead().\n\n"
                               "This test uses the source port and destination port.\n\n"
                               "The source port is opened only as write-only, and this\n"
                               "test from the port set control a signal bytesWritten().\n"
                               "In this case, count the number of emit signals bytesWritten()\n"
                               "and the number of bytes of data transmitted in the signal for\n"
                               "a single emit.\n\n"
                               "The destination port is opened as read-only, and this\n"
                               "test from the port set control a signal readyRead().\n"
                               "At the same time count the number of signals readyRead().\n\n"
                               "By default ports are opened in the mode: 9600 8 N 1, no flow control.\n"
                               "The testing process consists of several stages:\n\n"
                               "  Stage1. Opened and initialized ports. If an error occurs, it is\n"
                               "recorded in the log and testing is completed with failure. If\n"
                               "everything goes well, then go to stage 2.\n\n"
                               "  Stage2. Run single shot the timer interval to 1 second, and\n"
                               "further, the source port sends a byte, at the same time are tracked\n"
                               "and logged signals from the ports. Further, when the timer works,\n"
                               "there is a processing of the results of the signals. Compares the\n"
                               "number of bytes sent and received, and these results or an error is\n"
                               "generated with the termination of the test, or go to stage 3.\n\n"
                               "  Stage3. No different from stage 2 with the exception of the number\n"
                               "of bytes transmitted. Now write not one but several bytes. The number\n"
                               "of bytes transmitted is now increasing, according to the formula:\n"
                               "NumCurr = NumPrev + K, where K - some constant. Further, the\n"
                               "termination of a test or go to stage 4.\n\n"
                               "  Stage4. Does not differ from step 3, except the number of bytes.\n\n"
                               "  StageN. No different from the previous steps.\n\n"
                               "By default, the number of transactions (steps) for the transfer of\n"
                               "data is five, but this value can be changed by editing the source\n"
                               "code of the test. In the source code you can change settings such as:\n"
                               "- timeout of timer (by default 1 sec)\n"
                               "- number of steps (by default 5)\n"
                               "- initial number of bytes transmitted (by default 1 byte)\n"
                               "- constant growth rate K of transmitted bytes (by default 100 byte)."));

    m_srcPort = new SerialPort(this);
    m_dstPort = new SerialPort(this);

    connect(m_srcPort, SIGNAL(bytesWritten(qint64)),
            this, SLOT(procSignalBytesWritten(qint64)));
    connect(m_dstPort, SIGNAL(readyRead()),
            this, SLOT(procSignalReadyRead()));
}

/* Public slots */

void UnitTestSignals::start()
{
    QString header(tr("\n[ Test: ID#%1, Name: %2 ]\n%3\n\n"));
    header = header
            .arg(m_id)
            .arg(m_name)
            .arg(QString("timestamp"));/*.arg(UnitTestManager::timestamp());*/

    m_logger->addContent(header);

    m_srcPort->setPort(m_srcPortName);
    m_dstPort->setPort(m_dstPortName);

    if (!(open(UnitTestBase::SrcPort) && open(UnitTestBase::DstPort)
          && configure(UnitTestBase::SrcPort) && configure(UnitTestBase::DstPort))) {

        close(UnitTestBase::SrcPort);
        close(UnitTestBase::DstPort);
        emit error();
        return;
    } else {
        QString content(tr("\nSource and destination ports\n"
                           "opened as 9600 8 N 1 by default.\n"));
        m_logger->addContent(content);
    }

    // Prepare transaction begin.
    m_transactionNum = 0;
    m_bytesToWrite = MinBytesToWrite;
    m_bytesReallyWrited = 0;
    m_countSignalsBytesWritten = 0;
    m_countSignalsReadyRead = 0;

    transaction();
}

/* Private slots */

void UnitTestSignals::procSignalBytesWritten(qint64 bw)
{
    QString content(">signal bytesWritten(%1)\n");
    content = content.arg(bw);
    m_logger->addContent(content);
    ++m_countSignalsBytesWritten;
    m_bytesReallyWrited += bw;
}

void UnitTestSignals::procSignalReadyRead()
{
    ++m_countSignalsReadyRead;
}

void UnitTestSignals::procSingleShot()
{
    QByteArray data = m_dstPort->readAll();
    qint64 reallyRead = data.count();

    QString content(tr("- count signals bytesWritten : %1\n"
                       "- count signals readyRead    : %2\n"
                       "- bytes really write         : %3\n"
                       "- bytes really read          : %4\n"));
    content = content
            .arg(m_countSignalsBytesWritten)
            .arg(m_countSignalsReadyRead)
            .arg(m_bytesReallyWrited)
            .arg(reallyRead);

    m_logger->addContent(content);

    m_countSignalsBytesWritten = 0;
    m_countSignalsReadyRead = 0;

    if ((m_bytesReallyWrited != m_bytesToWrite)
            || (m_bytesToWrite != reallyRead)) {

        content = QString(tr("\nError: Mismatch of sent and received bytes.\n"));
        m_logger->addContent(content);
        close(UnitTestBase::SrcPort);
        close(UnitTestBase::DstPort);
        emit error();
    }

    m_bytesReallyWrited = 0;
    m_bytesToWrite += StepBytesToWrite;

    transaction();
}

void UnitTestSignals::transaction()
{
    if (m_transactionNum++ != TransactionLimit) {
        QString content(tr("\nTransaction #%1, bytes to write: %2\n"));
        content = content.arg(m_transactionNum).arg(m_bytesToWrite);
        m_logger->addContent(content);

        QByteArray data(m_bytesToWrite, qrand());
        QTimer::singleShot(TransactionMsecDelay, this, SLOT(procSingleShot()));
        m_srcPort->write(data);
    } else {
        close(UnitTestBase::SrcPort);
        close(UnitTestBase::DstPort);
        emit finished();
    }
}

/* Private */

bool UnitTestSignals::open(DirPorts dir)
{
    SerialPort *port = (dir == UnitTestBase::SrcPort) ?
                m_srcPort : m_dstPort;
    QIODevice::OpenMode mode = (dir == UnitTestBase::SrcPort) ?
                QIODevice::WriteOnly : QIODevice::ReadOnly;

    QString error("\nERROR: Can\'t open port %1\n");
    if (!port->open(mode)) {
        error = error.arg(port->portName());
        m_logger->addContent(error);
        return false;
    }
    return true;
}

bool UnitTestSignals::configure(DirPorts dir)
{
    SerialPort *port = (dir == UnitTestBase::SrcPort) ?
                m_srcPort : m_dstPort;

    if (!(port->setRate(9600) && port->setDataBits(SerialPort::Data8)
          && port->setParity(SerialPort::NoParity) && port->setStopBits(SerialPort::OneStop)
          && port->setFlowControl(SerialPort::NoFlowControl))) {

        QString error("\nError: Can\'t configure port %1\n");
        error = error.arg(port->portName());
        m_logger->addContent(error);
        return false;
    }
    return true;
}

void UnitTestSignals::close(DirPorts dir)
{
    if (dir == UnitTestBase::SrcPort) {
        if (m_srcPort->isOpen())
            m_srcPort->close();
    } else {
        if (m_dstPort->isOpen())
            m_dstPort->close();
    }
}


