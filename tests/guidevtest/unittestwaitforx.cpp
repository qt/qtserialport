#include "unittests.h"
#include "serialport.h"



/* Public methods */

UnitTestWaitForX::UnitTestWaitForX(Logger *logger, QObject *parent)
    : UnitTestBase(UnitTestBase::WaitForXUnitId, logger, parent)
{
    m_name = QString(tr("WaitForX Test"));
    m_description = QString(tr("\"WaitForX Test\" ..."));
}

/* Public slots */

void UnitTestWaitForX::start()
{
    QString header(tr("\n[ Test: ID#%1, Name: %2 ]\n%3\n\n"));
    header = header
            .arg(m_id)
            .arg(m_name)
            .arg(QString("timestamp"));/*.arg(UnitTestManager::timestamp());*/

    m_logger->addContent(header);

    ////

    emit finished();
}

