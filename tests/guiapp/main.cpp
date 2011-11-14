#include <QtGui/QApplication>
#include "maindialog.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainDialog dlg;
    dlg.show();

    return a.exec();
}
