#ifndef TRACEDIALOG_H
#define TRACEDIALOG_H

#include <QtGui/QDialog>

namespace Ui {
class TraceDialog;
}

class SerialPort;

class TraceDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TraceDialog(SerialPort *port, QWidget *parent = 0);
    ~TraceDialog();

protected:
    void changeEvent(QEvent *e);

private slots:
    void printTrace(const QByteArray &data, bool directionRx);
    void procSendButtonClick();
    void procClearButtonClick();
    void procReadyRead();

private:
    Ui::TraceDialog *ui;

    SerialPort *m_port;
};

#endif // TRACEDIALOG_H
