#include "unittestinfo.h"
#include "serialportinfo.h"

UnitTestInfo::UnitTestInfo(QObject *parent)
    : UnitTestBase(UnitTestBase::InfoUnitId, parent)
{
    m_name = QString(tr("Info Test"));
    m_description = QString(tr("Info Test Description"));
}

void UnitTestInfo::start()
{
    bool ret = openLog();
    if (!ret) {
        emit error();
        return;
    }

    QString header("Test: ID# %1, Name: %2, %3");
    header = header
            .arg(m_id)
            .arg(m_name)
            .arg(getDateTime());

    if (writeToLog(header)) {
        int it = 0;
        foreach (SerialPortInfo inf, SerialPortInfo::availablePorts()) {
            QString s("Port# %1, name: %2\n"
                      "  location    : %3\n"
                      "  description : %4\n"
                      "  valid       : %5\n"
                      "  busy        : %6");

            s = s
                    .arg(it++)
                    .arg(inf.portName())
                    .arg(inf.systemLocation())
                    .arg(inf.description())
                    .arg(inf.isValid())
                    .arg(inf.isBusy());

            ret = writeToLog(s);
            if (!ret)
                break;
        }

    }
    closeLog();

    if (ret)
        emit finished();
    else
        emit error();
}
