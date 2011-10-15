#include "unittestmanager.h"

#include <QtCore/QTimer>

/* Public methods */

bool UnitTestBase::isUse() const
{
    return false;
}

UnitTestBase *UnitTestFactory::create(UnitTypes unit)
{
    return 0;
}

UnitTestManager::UnitTestManager(QObject *parent)
    : QObject(parent)
    , m_count(0)
    , m_it(0)
{
    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(step()));

    // Create units.
    m_unitList.append(UnitTestFactory::create(UnitTestFactory::InfoTestUnit));


    m_count = m_unitList.count();
}

/* Public slots */

void UnitTestManager::start()
{
    m_it = 0;
    m_timer->start();
}

void UnitTestManager::stop()
{
    m_timer->stop();
}

/* Private slots */

 void UnitTestManager::step()
 {

 }
