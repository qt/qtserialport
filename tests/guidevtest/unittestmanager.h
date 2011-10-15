#ifndef UNITTESTMANAGER_H
#define UNITTESTMANAGER_H

#include <QtCore/QObject>

class QTimer;
class UnitTestManager;

class UnitTestBase
{
public:
    bool isUse() const;

};

class UnitTestFactory
{
public:
    enum UnitTypes {
        InfoTestUnit,

    };

    static UnitTestBase *create(UnitTypes unit);
};

class UnitTestManager : public QObject
{
    Q_OBJECT
public:
    explicit UnitTestManager(QObject *parent = 0);

signals:
    void started();
    void stopped();

public slots:
    void start();
    void stop();

private slots:
    void step();

private:
    int m_count;
    int m_it;
    QTimer *m_timer;
    QList<UnitTestBase *> m_unitList;

};

#endif // UNITTESTMANAGER_H
