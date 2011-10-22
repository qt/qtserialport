#include "unittests.h"
#include "serialport.h"



/* Public methods */

UnitTestSignals::UnitTestSignals(Logger *logger, QObject *parent)
    : UnitTestBase(UnitTestBase::InfoUnitId, logger, parent)
{
    m_name = QString(tr("Signals Test"));
    m_description = QString(tr("Signals Test Description"));

    m_srcPort = new SerialPort(this);
    m_dstPort = new SerialPort(this);
}

/* Public slots */

void UnitTestSignals::start()
{
    m_srcPort->setPort(m_srcPortName);
    m_dstPort->setPort(m_dstPortName);

    ///

    emit finished();
}

/* Private slots */

void UnitTestSignals::stage()
{

}


