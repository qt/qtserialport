#ifndef MAINDIALOG_H
#define MAINDIALOG_H

#include <QtGui/QDialog>

namespace Ui {
class MainDialog;
}

class SerialPort;
class QTimer;

class MainDialog : public QDialog
{
    Q_OBJECT
public:
    explicit MainDialog(QWidget *parent = 0);
    ~MainDialog();

protected:
    void changeEvent(QEvent *e);

private slots:
    void procShowPorts();
    void procItemPortChanged(int idx);

    void procControlButtonClick();
    void procOptionsButtonClick();
    void procIOButtonClick();
    void procRtsButtonClick();
    void procDtrButtonClick();

    void procUpdateLines();

private:
    Ui::MainDialog *ui;

    SerialPort *m_port;
    QTimer *m_timer;

    bool m_rts;
    bool m_dtr;

    void fillOpenModeComboBox();

};

#endif // MAINDIALOG_H
