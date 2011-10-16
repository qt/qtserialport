#include "testsdialog.h"
#include "ui_testsdialog.h"
#include "unittestmanager.h"



// TestsViewModel

/* Public methods */

TestsViewModel::TestsViewModel(UnitTestManager *manager, QObject *parent)
    : QAbstractListModel(parent)
{
    m_unitList = manager->units();
}

int TestsViewModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_unitList.count();
}

QVariant TestsViewModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid()
            && (index.row() < m_unitList.count())) {
        UnitTestBase *item = m_unitList.at(index.row());
        if (role == Qt::DisplayRole)
            return item->name();
        if (role == Qt::CheckStateRole)
            return item->isEnabled() ? Qt::Checked : Qt::Unchecked;
    }
    return QVariant();
}

QVariant TestsViewModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(section);
    Q_UNUSED(orientation);
    Q_UNUSED(role);
    return QVariant();
}

Qt::ItemFlags TestsViewModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flag = Qt::ItemIsEnabled;
    if (index.isValid())
        flag |= Qt::ItemIsUserCheckable | Qt::ItemIsSelectable;
    return flag;
}

bool TestsViewModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid()) {
        UnitTestBase *item = m_unitList.at(index.row());
        if (role == Qt::CheckStateRole) {
            bool enable = value.toBool();
            if (item->isEnabled() != enable) {
                item->setEnable(enable);
                emit dataChanged(index, index);
                return true;
            }
        }
    }
    return false;
}



// TestsDialog

/* Public methods */

TestsDialog::TestsDialog(UnitTestManager *manager)
    : ui(new Ui::TestsDialog)
{
    ui->setupUi(this);
    m_model = new TestsViewModel(manager, this);
    ui->listView->setModel(m_model);
}

TestsDialog::~TestsDialog()
{
    delete ui;
}
