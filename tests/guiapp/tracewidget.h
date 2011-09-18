#ifndef TRACEWIDGET_H
#define TRACEWIDGET_H

#include <QtGui/QWidget>

namespace Ui {
class TraceWidget;
}

class SerialPort;

class TraceWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TraceWidget(SerialPort *port, QWidget *parent = 0);
    ~TraceWidget();

protected:
    void changeEvent(QEvent *e);

private slots:
    void printTrace(const QByteArray &data, bool directionRx);
    void procSendButtonClick();
    void procClearButtonClick();
    void procReadyRead();

private:
    Ui::TraceWidget *ui;

    SerialPort *m_port;
};

#endif // TRACEWIDGET_H
