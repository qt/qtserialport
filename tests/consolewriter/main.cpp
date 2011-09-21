/*
* ConsoleWriter
*
* This application is part of the examples on the use of the library QSerialDevice.
*
* ConsoleWriter - a test console application to write data to the port.
*
* Copyright (C) 2011  Denis Shienkov
*
* Contact Denis Shienkov:
*          e-mail: <scapig2@yandex.ru>
*             ICQ: 321789831
*/

#include <iostream>
#include <QtCore/QCoreApplication>

#include "serialport.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // 1. First - create an instance of an object.
    SerialPort port;

    char inbuf[30];

    std::cout << "Please enter serial device name,\n"
                 "specific by OS, example\n"
                 "- in Windows: COMn\n"
                 "- in GNU/Linux: ttyXYZn\n"
                 ":";
    std::cin >> inbuf;

    // 2. Second - set the device name.
    port.setPort(QString(inbuf));

    std::cout << "The port will be opened in write-only mode (QIODevice::WriteOnly).\n"
                 "But you can choose to buffered or not (QIODevice::Unbuffered).\n"
                 "To understand what is the difference - try to change these modes!\n"
                 "Disable buffering [y/N] ?:";
    std::cin >> inbuf;

    QIODevice::OpenMode mode = QIODevice::WriteOnly;
    if (inbuf[0] == 'y')
        mode |= QIODevice::Unbuffered;

    // 3. Third - open the device.
    if (port.open(mode)) {

        // 4. Fourth - now you can configure it (only after successfully opened!).
        if (port.setRate(115200) && port.setDataBits(SerialPort::Data8)
                && port.setParity(SerialPort::NoParity) && port.setStopBits(SerialPort::OneStop)
                && port.setFlowControl(SerialPort::NoFlowControl)) {

            // 5. Fifth - you can now read/write device, or further modify its settings, etc.
            while (1) {

                int len = 0;
                std::cout << "Please enter len data for write, bytes: ";
                std::cin >> len;

                QByteArray data(len, 0);

                if (port.write(data)) {
                    std::cout << "Writed " << data.size() << " bytes" << std::endl;
                } else {
                    std::cout << "Write fail." << std::endl;
                }
            }
        } else {
            std::cout << "Configure " << port.portName().toLocal8Bit().constData() << " fail.";
            port.close();
        }
    } else {
        std::cout << "Open " << port.portName().toLocal8Bit().constData() << " fail.";
    }

    return app.exec();
}
