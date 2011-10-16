#ifndef MAINDIALOG_H
#define MAINDIALOG_H

#include <QtGui/QDialog>
#include <QtCore/QMap>



namespace Ui {
class MainDialog;
}

class QState;
class QStateMachine;
class UnitTestManager;

class MainDialog : public QDialog
{
    Q_OBJECT
signals:
    void toIdle();

public:
    explicit MainDialog(QWidget *parent = 0);
    ~MainDialog();

private slots:
    void procIdleState();
    void procOptionsState();
    void procRunningState();
    void procUbdatePortsBox();

private:
    Ui::MainDialog *ui;

    QState *m_idleState;
    QState *m_optionsState;
    QState *m_runningState;
    QStateMachine *m_stateMachine;
    UnitTestManager *m_utManager;
};

#endif // MAINDIALOG_H
