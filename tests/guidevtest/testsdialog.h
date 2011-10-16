#ifndef TESTSDIALOG_H
#define TESTSDIALOG_H

#include <QtGui/QDialog>
#include <QtCore/QAbstractListModel>



namespace Ui {
class TestsDialog;
}

class UnitTestManager;
class UnitTestBase;

class TestsViewModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit TestsViewModel(UnitTestManager *manager, QObject *parent = 0);
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation,
                                int role = Qt::DisplayRole) const;
    virtual  Qt::ItemFlags flags(const QModelIndex &index) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value,
                         int role = Qt::EditRole);

private:
    QList<UnitTestBase *> m_unitList;
};



class TestsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TestsDialog(UnitTestManager *manager);
    ~TestsDialog();

private:
    Ui::TestsDialog *ui;
    TestsViewModel *m_model;
};

#endif // TESTSDIALOG_H
