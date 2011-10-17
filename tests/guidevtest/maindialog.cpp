#include "maindialog.h"
#include "ui_maindialog.h"

#include <QtCore/QStateMachine>
#include <QtGui/QMessageBox>

#include "serialportinfo.h"
#include "unittestmanager.h"
#include "testsdialog.h"



/* Public methods */

MainDialog::MainDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::MainDialog)
{
    ui->setupUi(this);
    m_utManager = new UnitTestManager(this);

    m_idleState = new QState();
    m_optionsState = new QState();
    m_runningState = new QState();

    connect(m_idleState,SIGNAL(entered()), this, SLOT(procIdleState()));
    connect(m_optionsState,SIGNAL(entered()), this, SLOT(procOptionsState()));
    connect(m_runningState,SIGNAL(entered()), this, SLOT(procRunningState()));

    // From Idle State to ...
    m_idleState->addTransition(ui->optButton, SIGNAL(clicked()), m_optionsState);
    m_idleState->addTransition(ui->ctrlButton, SIGNAL(clicked()), m_runningState);

    // From Options State to ...
    m_optionsState->addTransition(this, SIGNAL(toIdle()), m_idleState);

    // From Running State to ...
    m_runningState->addTransition(ui->ctrlButton, SIGNAL(clicked()), m_idleState);
    m_runningState->addTransition(this, SIGNAL(toIdle()), m_idleState);
    m_runningState->addTransition(m_utManager, SIGNAL(finished()), m_idleState);

    m_stateMachine = new QStateMachine(this);
    m_stateMachine->addState(m_idleState);
    m_stateMachine->addState(m_optionsState);
    m_stateMachine->addState(m_runningState);

    m_stateMachine->setInitialState(m_idleState);
    m_stateMachine->start();
}

MainDialog::~MainDialog()
{
    delete ui;
}

/* Private slots */

void MainDialog::procIdleState()
{
    ui->ctrlButton->setText(tr("Start"));
    ui->srcBox->setEnabled(true);
    ui->dstBox->setEnabled(true);
    ui->optButton->setEnabled(true);
    procUbdatePortsBox();
}

void MainDialog::procOptionsState()
{
    TestsDialog dlg(m_utManager);
    //dlg.setModal(true);
    dlg.exec();

    emit toIdle();
}

void MainDialog::procRunningState()
{
    if (ui->srcBox->currentText() == ui->dstBox->currentText()) {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText(tr("You can not choose the source and\n"
                          "destination the same name."));
        msgBox.exec();
        emit toIdle();
        return;
    }
    ui->ctrlButton->setText(tr("Stop"));
    ui->srcBox->setEnabled(false);
    ui->dstBox->setEnabled(false);
    ui->optButton->setEnabled(false);

    m_utManager->setLogFileName(qApp->applicationDirPath() + "/Test.log");
    m_utManager->setPorts(ui->srcBox->currentText(),
                          ui->dstBox->currentText());
    m_utManager->start();
}

void MainDialog::procUbdatePortsBox()
{
    ui->srcBox->clear();
    ui->dstBox->clear();

    foreach(SerialPortInfo inf, SerialPortInfo::availablePorts()) {
        if (inf.isValid() && !inf.isBusy()) {
            QString s = inf.portName();
            ui->srcBox->addItem(s);
            ui->dstBox->addItem(s);
        }
    }
}


/* Private methods */
