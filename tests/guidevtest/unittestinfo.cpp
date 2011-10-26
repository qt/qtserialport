#include "unittests.h"
#include "serialportinfo.h"



/* Public methods */

UnitTestInfo::UnitTestInfo(Logger *logger, QObject *parent)
    : UnitTestBase(UnitTestBase::InfoUnitId, logger, parent)
{
    m_name = QString(tr("Info Test"));
    m_description = QString(tr("\"Info Test\" tested class SerialPortInfo,\n"
                               "by calling its methods and write the results to a log.\n\n"
                               "In the log lists all serial ports that were discovered,\n"
                               "their properties, states, and supported standard rates."));
}

/* Public slots */

void UnitTestInfo::start()
{
    QString header(tr("\n[ Test: ID#%1, Name: %2 ]\n%3\n\n"));
    header = header
            .arg(m_id)
            .arg(m_name)
            .arg(QString("timestamp"));/*.arg(UnitTestManager::timestamp());*/

    m_logger->addContent(header);

    int it = 0;
    foreach (SerialPortInfo inf, SerialPortInfo::availablePorts()) {
        QString body(tr("Port# %1, name : %2\n"
                        "  location    : %3\n"
                        "  description : %4\n"
                        "  valid       : %5\n"
                        "  busy        : %6\n"
                        "  rates       : %7\n"));

        QString r;
        foreach (qint32 rate, inf.standardRates()) {
            r.append(QString::number(rate));
            r.append(';');
        }

        body = body
                .arg(it++)
                .arg(inf.portName())
                .arg(inf.systemLocation())
                .arg(inf.description())
                .arg(inf.isValid())
                .arg(inf.isBusy())
                .arg(r);

        m_logger->addContent(body);
    }

    QString trailer(tr("\nFound %1 ports.\n"));
    trailer = trailer.arg(it);
    m_logger->addContent(trailer);

    emit finished();
}
