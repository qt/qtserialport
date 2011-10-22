#include <QtGui/QApplication>

#include "maindialog.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QCoreApplication::setOrganizationName("Kuzulis");
    QCoreApplication::setOrganizationDomain("kuzulis.com");
    QCoreApplication::setApplicationName("QSerialDevice unit test");

    MainDialog w;
    w.show();

    return a.exec();
}
