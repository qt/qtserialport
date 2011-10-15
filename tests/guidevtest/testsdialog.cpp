#include "testsdialog.h"
#include "ui_testsdialog.h"

TestsDialog::TestsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::TestsDialog)
{
    ui->setupUi(this);
}

TestsDialog::~TestsDialog()
{
    delete ui;
}
