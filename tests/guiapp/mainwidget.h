#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QtGui/QWidget>

namespace Ui {
class MainWidget;
}

class SerialPort;
class OptionsWidget;
class TraceWidget;
class QTimer;

class MainWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MainWidget(QWidget *parent = 0);
    ~MainWidget();

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
    Ui::MainWidget *ui;
    OptionsWidget *m_optionsWidget;
    TraceWidget *m_traceWidget;
    SerialPort *m_port;
    QTimer *m_timer;

    bool m_rts;
    bool m_dtr;

    void fillOpenModeComboBox();

};

#endif // MAINWIDGET_H
