#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QLabel>
#include <QtAddOnSerialPort/serialportinfo.h>

QT_USE_NAMESPACE_SERIALPORT

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QWidget w;
    w.setWindowTitle(QObject::tr("Info about all available serial ports."));
    QVBoxLayout *layout = new QVBoxLayout;

    foreach (const SerialPortInfo &info, SerialPortInfo::availablePorts()) {
        QString s(QObject::tr("Port: %1\n"
                              "Location: %2\n"
                              "Description: %3\n"
                              "Manufacturer: %4\n"
                              "Vid: %5\n"
                              "Pid: %6\n"
                              "Busy: %7\n"));

        s = s.arg(info.portName()).arg(info.systemLocation())
                .arg(info.description()).arg(info.manufacturer())
                .arg(info.vid()).arg(info.pid())
                .arg(info.isBusy() ? QObject::tr("Yes") : QObject::tr("No"));

        QLabel *label = new QLabel(s);
        layout->addWidget(label);
    }

    w.setLayout(layout);
    w.show();

    return a.exec();
}
