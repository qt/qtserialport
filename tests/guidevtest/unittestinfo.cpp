#include "unittestinfo.h"
#include "serialportinfo.h"



/* Public methods */

UnitTestInfo::UnitTestInfo(QObject *parent)
    : UnitTestBase(UnitTestBase::InfoUnitId, parent)
{
    m_name = QString(tr("Info Test"));
    m_description = QString(tr("Info Test Description"));
}

/* Public slots */

void UnitTestInfo::start()
{
    bool ret = UnitTestManager::openLog();
    if (!ret) {
        emit error();
        return;
    }

    QString header(tr("> Test: ID#%1, Name: %2 \n%3\n\n"));
    header = header
            .arg(m_id)
            .arg(m_name)
            .arg(UnitTestManager::timestamp());

    if (UnitTestManager::writeToLog(header)) {
        int it = 0;
        foreach (SerialPortInfo inf, SerialPortInfo::availablePorts()) {
            QString s(tr("Port# %1, name : %2\n"
                      "  location    : %3\n"
                      "  description : %4\n"
                      "  valid       : %5\n"
                      "  busy        : %6\n"));

            s = s
                    .arg(it++)
                    .arg(inf.portName())
                    .arg(inf.systemLocation())
                    .arg(inf.description())
                    .arg(inf.isValid())
                    .arg(inf.isBusy());

            ret = UnitTestManager::writeToLog(s);
            if (!ret)
                break;
        }
    }
    UnitTestManager::closeLog();

    if (ret)
        emit finished();
    else
        emit error();
}
