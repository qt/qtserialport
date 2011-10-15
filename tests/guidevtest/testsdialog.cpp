#include "testsdialog.h"
#include "ui_testsdialog.h"
#include "unittestmanager.h"

TestsDialog::TestsDialog(UnitTestManager *manager)
    : ui(new Ui::TestsDialog)
    , m_utManager(manager)
{
    ui->setupUi(this);
}

TestsDialog::~TestsDialog()
{
    delete ui;
}
