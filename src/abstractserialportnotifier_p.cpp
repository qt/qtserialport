#include "serialportnotifier_p.h"
#include "serialport_p.h"

AbstractSerialPortNotifier *AbstractSerialPortNotifier::createNotifier(SerialPortPrivate *parent)
{
    return new SerialPortNotifier(parent);
}

void AbstractSerialPortNotifier::deleteNotifier(AbstractSerialPortNotifier *notifier)
{
    if (notifier)
        delete notifier;
}
