#include "unittests.h"
#include "serialport.h"



/* Public methods */

UnitTestSignals::UnitTestSignals(Logger *logger, QObject *parent)
    : UnitTestBase(UnitTestBase::SignalsUnitId, logger, parent)
{
    m_name = QString(tr("Signals Test"));
    m_description = QString(tr("Signals Test Description"));

    m_srcPort = new SerialPort(this);
    m_dstPort = new SerialPort(this);
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
    }

    ////// !!! Implement me

    emit finished();
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
          && port->setFlowControl(SerialPort::NoFlowControl) && port->setDataErrorPolicy())) {

        QString error("\nERROR: Can\'t configure port %1\n");
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

