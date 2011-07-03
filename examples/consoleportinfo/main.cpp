#include <QtCore/QCoreApplication>
#include <QtCore/QStringList>
#include <QtCore/QDebug>

#include "serialportinfo.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QList<SerialPortInfo> list = SerialPortInfo::availablePorts();
    int item = 0;
    foreach(SerialPortInfo info, list) {
        qDebug() << "--> Item: " << item++;
        qDebug() << "Port name        : " << info.portName();
        qDebug() << "Port location    : " << info.systemLocation();
        qDebug() << "Port description : " << info.description();
        qDebug() << "Port manufacturer: " << info.manufacturer();
    }

    return a.exec();
}
