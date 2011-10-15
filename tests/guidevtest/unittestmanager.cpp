#include "unittestmanager.h"
#include "unittestinfo.h"

#include <QtCore/QTimer>
#include <QtCore/QSettings>
#include <QtCore/QDateTime>


const QString UnitTestManager::logFileParam = "UnitTestManager/logFile";


// UnitTestBase

/* Public methods */

UnitTestBase::UnitTestBase(UnitID id, QObject *parent)
    : QObject(parent)
    , m_id(id)
{
    m_enableParam = "%1/enable";
}

void UnitTestBase::setPorts(const QString &src, const QString &dst)
{
    m_srcPort = src;
    m_dstPort = dst;
}

void UnitTestBase::setEnable(bool enable)
{
    QSettings settings;
    settings.setValue(m_enableParam.arg(m_id), enable);
}

bool UnitTestBase::isEnabled() const
{
    QSettings settings;
    return settings.value(m_enableParam.arg(m_id)).toBool();
}

int UnitTestBase::id() const
{
    return m_id;
}

QString UnitTestBase::name() const
{
    return m_name;
}

QString UnitTestBase::description() const
{
    return m_description;
}

/* Protected methods */

bool UnitTestBase::openLog()
{
    QSettings settings;
    m_log.setFileName(settings.value(UnitTestManager::logFileParam).toString());
    return (m_log.exists() && m_log.open(QIODevice::WriteOnly | QIODevice::Append));
}

bool UnitTestBase::writeToLog(const QString &content)
{
    return (m_log.write(content.toLocal8Bit()) > 0);
}

void UnitTestBase::closeLog()
{
    m_log.close();
}

QString UnitTestBase::getDateTime() const
{
    return QDateTime::currentDateTime().toString();
}



// UnitTestFactory

/* Public methods */

UnitTestBase *UnitTestFactory::create(UnitTestBase::UnitID id)
{
    switch (id) {
    case UnitTestBase::InfoUnitId:
        return new UnitTestInfo();
    default:;
    }

    return 0;
}



// UnitTestManager

/* Public methods */

UnitTestManager::UnitTestManager(QObject *parent)
    : QObject(parent)
    , m_count(0)
    , m_it(0)
{
    // Create all units.
    m_unitList.append(UnitTestFactory::create(UnitTestBase::InfoUnitId));
    //..
    //...

    foreach (UnitTestBase *unit, m_unitList)
        connect(unit, SIGNAL(finished()), this, SLOT(step()));

    m_count = m_unitList.count();
}

void UnitTestManager::setPorts(const QString &src, const QString &dst)
{
    m_srcPort = src;
    m_dstPort = dst;
}

void UnitTestManager::setLogFile(const QString &log)
{

}

QList<UnitTestBase *> UnitTestManager::units() const
{
    return m_unitList;
}



/* Public slots */

void UnitTestManager::start()
{
    m_it = 0;
    step();
}

void UnitTestManager::stop()
{
}

/* Private slots */

void UnitTestManager::step()
{
    if (m_it == m_count) {
        emit allFinished();
        return;
    }

    UnitTestBase *unit = m_unitList.at(m_it++);
    if (unit->isEnabled()) {
        unit->setPorts(m_srcPort, m_dstPort);
        QTimer::singleShot(1000, unit, SLOT(start()));
    }
    else
        step();
}
