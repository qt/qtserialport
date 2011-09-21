#ifndef PORTINFODIALOG_H
#define PORTINFODIALOG_H

#include <QDialog>

namespace Ui {
    class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();

private slots:
    void procUpdateAvailablePorts();
    void procItemPortChanged(int idx);
    void procBusyButtonClick();
    void procValidButtonClick();
    void procRatesButtonClick();

private:
    Ui::Dialog *ui;
};

#endif // PORTINFODIALOG_H
