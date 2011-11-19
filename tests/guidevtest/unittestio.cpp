#include "unittests.h"
#include "serialport.h"

#include <QtCore/QTimer>
#include <QtCore/QStringList>


enum {
    RATES_COUNT    = 2,
    DATABITS_COUNT = 1,
    PARITY_COUNT   = 5,
    STOPBITS_COUNT = 2,
    FLOW_COUNT     = 3
};

static const SerialPort::Rate vratesarray[RATES_COUNT] = {
    SerialPort::Rate9600,
    SerialPort::Rate115200
};
static const char *sratesarray[] = {
    "9600\0",
    "115200\0"
};

static const SerialPort::DataBits vdatabitsarray[DATABITS_COUNT] = {
    SerialPort::Data8
};
static const char *sdatabitsarray[] = {
    "8\0"
};

static const SerialPort::Parity vparitysarray[PARITY_COUNT] = {
    SerialPort::NoParity,
    SerialPort::EvenParity,
    SerialPort::OddParity,
    SerialPort::SpaceParity,
    SerialPort::MarkParity
};
static const char *sparitysarray[] = {
    "none\0",
    "even\0",
    "odd\0",
    "space\0",
    "mark\0"
};

static const SerialPort::StopBits vstopbitsarray[STOPBITS_COUNT] = {
    SerialPort::OneStop,
    SerialPort::TwoStop
};
static const char *sstopbitsarray[] = {
    "one\0",
    "two\0"
};

static const SerialPort::FlowControl vflowsarray[FLOW_COUNT] = {
    SerialPort::NoFlowControl,
    SerialPort::HardwareControl,
    SerialPort::SoftwareControl
};
static const char *sflowsarray[] = {
    "none\0",
    "hardware\0",
    "software\0"
};



/* Public methods */

UnitTestIO::UnitTestIO(Logger *logger, QObject *parent)
    : UnitTestBase(UnitTestBase::IOUnitId, logger, parent)
    , m_rateIterator(0)
    , m_databitsIterator(0)
    , m_parityIterator(0)
    , m_stopbitsIterator(0)
    , m_flowIterator(0)
    , m_bytesWrite(0)
    , m_bytesRead(0)
{
    m_name = QString(tr("IO Test"));
    m_description = QString(tr("\"IO Test\" designed to test the I/O between the two ports\n"
                               "Source port sends a data packet to the destination port,\n"
                               "that reads the packet.\n"
                               "  The default packet size is 500 bytes, the size can be changed\n"
                               "programmatically by changing the value of the\n"
                               "variable TransferBytesCount. Also, before sending the package\n"
                               "is filled with a random value.\n"
                               "  Both ports after each transaction, change their parameters:\n"
                               "speed, number of data bits, parity, number of stop bits,\n"
                               "flow regime, until the end all enumerated parameters.\n"
                               "After each transaction is recorded in a log the contents of the\n"
                               "sent and received packet, and check their size. If the packet\n"
                               "sizes are different, the test is aborted with an error which is\n"
                               "recorded in the log.\n"
                               ));

    m_srcPort = new SerialPort(this);
    m_dstPort = new SerialPort(this);
}

/* Public slots */

void UnitTestIO::start(bool first)
{
    if (first) {
        QString header(tr("\n[ Test: ID#%1, Name: %2 ]\n%3\n\n"));
        header = header
                .arg(m_id)
                .arg(m_name)
                .arg(QString("timestamp"));/*.arg(UnitTestManager::timestamp());*/

        m_logger->addContent(header);

        m_srcPort->setPort(m_srcPortName);
        m_dstPort->setPort(m_dstPortName);

        if (!(open(UnitTestBase::SrcPort) && open(UnitTestBase::DstPort))) {
            emit error();
            return;
        } else {
            QString content(tr("\nSource and destination ports is opened.\n"));
            m_logger->addContent(content);
        }

        m_rateIterator = 0;
        m_databitsIterator = 0;
        m_parityIterator = 0;
        m_stopbitsIterator = 0;
        m_flowIterator = 0;
    }

    transaction();
}

/* Private slots */

void UnitTestIO::procSingleShot()
{
    QByteArray data = m_dstPort->readAll();
    m_bytesRead = data.count();

    QString content(tr("= write: %1 read: %2 =\n"));
    content = content
            .arg(m_bytesWrite)
            .arg(m_bytesRead);
    m_logger->addContent(content);

    if (m_bytesWrite != m_bytesRead) {
        content = QString(tr("\nError: Mismatch of write and read bytes.\n"));
        m_logger->addContent(content);
        close(UnitTestBase::SrcPort);
        close(UnitTestBase::DstPort);
        emit error();
        return;
    }

    ++m_rateIterator;
    if (m_rateIterator == RATES_COUNT) {
        m_rateIterator = 0;

        ++m_databitsIterator;
        if (m_databitsIterator == DATABITS_COUNT) {
            m_databitsIterator = 0;

            ++m_parityIterator;
            if (m_parityIterator == PARITY_COUNT) {
                m_parityIterator = 0;

                ++m_stopbitsIterator;
                if (m_stopbitsIterator == STOPBITS_COUNT) {
                    m_stopbitsIterator = 0;

                    ++m_flowIterator;
                    if (m_flowIterator == FLOW_COUNT) {
                        m_flowIterator = 0;

                        close(UnitTestBase::SrcPort);
                        close(UnitTestBase::DstPort);
                        emit finished();
                        return;
                    }
                }
            }
        }
    }

    transaction();
}

void UnitTestIO::transaction()
{
    if (!(configure(UnitTestBase::SrcPort) && configure(UnitTestBase::DstPort))) {
        emit error();
        return;
    }

    QString content(tr("\nrate    : %1"
                       "\ndatabits: %2"
                       "\npatity  : %3"
                       "\nstopbits: %4"
                       "\nflow    : %5\n"));

    content = content
            .arg(QString(sratesarray[m_rateIterator]))
            .arg(QString(sdatabitsarray[m_databitsIterator]))
            .arg(QString(sparitysarray[m_parityIterator]))
            .arg(QString(sstopbitsarray[m_stopbitsIterator]))
            .arg(QString(sflowsarray[m_flowIterator]));

    m_logger->addContent(content);

    QByteArray data(TransferBytesCount, qrand());
    m_bytesWrite = m_srcPort->write(data);

    QTimer::singleShot(TransactionMsecDelay, this, SLOT(procSingleShot()));
}

/* Private */

bool UnitTestIO::open(DirPorts dir)
{
    SerialPort *port = (dir == UnitTestBase::SrcPort) ?
                m_srcPort : m_dstPort;
    QIODevice::OpenMode mode = (dir == UnitTestBase::SrcPort) ?
                QIODevice::WriteOnly : QIODevice::ReadOnly;

    QString error("\nError: Can\'t open port %1\n");
    if (!port->open(mode)) {
        error = error.arg(port->portName());
        m_logger->addContent(error);
        return false;
    }
    return true;
}

bool UnitTestIO::configure(DirPorts dir)
{
    SerialPort *port = (dir == UnitTestBase::SrcPort) ?
                m_srcPort : m_dstPort;

    if (!(port->setRate(vratesarray[m_rateIterator])
          && port->setDataBits(vdatabitsarray[m_databitsIterator])
          && port->setParity(vparitysarray[m_parityIterator])
          && port->setStopBits(vstopbitsarray[m_stopbitsIterator])
          && port->setFlowControl(vflowsarray[m_flowIterator]))) {

        QString error("\nError: Can\'t configure port %1\n");
        error = error.arg(port->portName());
        m_logger->addContent(error);
        return false;
    }
    return true;
}

void UnitTestIO::close(DirPorts dir)
{
    if (dir == UnitTestBase::SrcPort) {
        if (m_srcPort->isOpen())
            m_srcPort->close();
    } else {
        if (m_dstPort->isOpen())
            m_dstPort->close();
    }
}




