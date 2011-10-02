/*
    License...
*/

#include "serialportnotifier_p.h"
#include "serialport_p.h"

//#include <QtCore/QDebug>


/* Public methods */


SerialPortNotifier::SerialPortNotifier(QObject *parent)
    : QObject(parent)
{
    // Impl me
}

SerialPortNotifier::~SerialPortNotifier()
{
    // Impl me
}

bool SerialPortNotifier::isReadNotificationEnabled() const
{
    // Impl me
    return false;
}

void SerialPortNotifier::setReadNotificationEnabled(bool enable)
{
    Q_ASSERT(m_ref);
    // Impl me
}

bool SerialPortNotifier::isWriteNotificationEnabled() const
{
    // Impl me
    return false;
}

void SerialPortNotifier::setWriteNotificationEnabled(bool enable)
{
    Q_ASSERT(m_ref);
    // Impl me
}


/* Protected methods */






