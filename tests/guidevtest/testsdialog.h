#ifndef TESTSDIALOG_H
#define TESTSDIALOG_H

#include <QDialog>

namespace Ui {
    class TestsDialog;
}

class TestsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TestsDialog(QWidget *parent = 0);
    ~TestsDialog();

private:
    Ui::TestsDialog *ui;
};

#endif // TESTSDIALOG_H
