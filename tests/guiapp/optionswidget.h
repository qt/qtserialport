#ifndef OPTIONSWIDGET_H
#define OPTIONSWIDGET_H

#include <QtGui/QWidget>

namespace Ui {
class OptionsWidget;
}

class SerialPort;

class OptionsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit OptionsWidget(SerialPort *port, QWidget *parent = 0);
    ~OptionsWidget();

protected:
    void showEvent(QShowEvent *e);

private slots:
    void procFillingOptions();
    void procApplyButtonClick();

private:
    Ui::OptionsWidget *ui;
    SerialPort *m_port;
    int m_rate;
    int m_data;
    int m_parity;
    int m_stop;
    int m_flow;
    int m_policy;

    void detectOptions();
};

#endif // OPTIONSWIDGET_H
