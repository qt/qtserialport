#ifndef TESTSDIALOG_H
#define TESTSDIALOG_H

#include <QtGui/QDialog>

namespace Ui {
    class TestsDialog;
}

class UnitTestManager;

class TestsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TestsDialog(UnitTestManager *manager);
    ~TestsDialog();

private:
    Ui::TestsDialog *ui;
    UnitTestManager *m_utManager;
};

#endif // TESTSDIALOG_H
