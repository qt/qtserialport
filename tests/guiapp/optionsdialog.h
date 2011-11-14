#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QtGui/QDialog>

namespace Ui {
class OptionsDialog;
}

class SerialPort;

class OptionsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit OptionsDialog(SerialPort *port, QWidget *parent = 0);
    ~OptionsDialog();

protected:
    void showEvent(QShowEvent *e);

private slots:
    void procFillingOptions();
    void procApplyButtonClick();

private:
    Ui::OptionsDialog *ui;
    SerialPort *m_port;
    int m_rate;
    int m_data;
    int m_parity;
    int m_stop;
    int m_flow;
    int m_policy;

    void detectOptions();
};

#endif // OPTIONSDIALOG_H
