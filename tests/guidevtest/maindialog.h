#ifndef MAINDIALOG_H
#define MAINDIALOG_H

#include <QtGui/QDialog>
#include <QtCore/QAbstractListModel>



namespace Ui {
class MainDialog;
}

class UnitTestBase;

class TestsViewModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit TestsViewModel(const QList<UnitTestBase *> &list, QObject *parent = 0);
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation,
                                int role = Qt::DisplayRole) const;
    virtual  Qt::ItemFlags flags(const QModelIndex &index) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value,
                         int role = Qt::EditRole);
    virtual QModelIndex index(int row, int column,
                              const QModelIndex &parent = QModelIndex()) const;

private:
    QList<UnitTestBase *> m_testsList;
};


class DescriptionDialog : public QDialog
{
public:
    explicit DescriptionDialog(const QString &content, QWidget *parent = 0);
};


class Logger;
class QModelIndex;

class MainDialog : public QDialog
{
    Q_OBJECT
public:
    explicit MainDialog(QWidget *parent = 0);
    ~MainDialog();

private slots:
    void procLogChanged(const QString &log);
    void procClearLogOnStartChanged(bool enable);
    void procBreakAllOnErrorChanged(bool enable);

    void procStartButtonClick();
    void procTestStarted();
    void procTestFinished();
    void procTestError();

    void procItemDoubleClick(const QModelIndex &index);

private:
    Ui::MainDialog *ui;
    TestsViewModel *m_model;
    QList<UnitTestBase *> m_testsList;
    Logger *m_logger;
    int m_enabledTestsCount;
    int m_it;

    static const QString logFileSettingsKey;
    static const QString breakOnErrorSettingsKey;
    static const QString clearLogOnStartSettingsKey;

    void showSettings();
    void createAvailableTests();
    void fillPairs();
    void enableUi(bool enable);
};

#endif // MAINDIALOG_H
