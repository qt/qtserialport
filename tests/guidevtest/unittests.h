#ifndef UNITTESTS_H
#define UNITTESTS_H

#include <QtCore/QObject>


class QFile;

class Logger : public QObject
{
    Q_OBJECT
public:
    explicit Logger(QObject *parent = 0);
    void setFileName(const QString &name);
    void addContent(const QString &content, bool clearAll = false);

private:
    QFile *m_file;
};

class UnitTestBase : public QObject
{
    Q_OBJECT
signals:
    void finished();
    void error();

public:
    enum UnitID {
        InfoUnitId,

    };

    explicit UnitTestBase(UnitID id, Logger *logger, QObject *parent = 0);
    void setPair(const QString &src, const QString &dst);
    void setEnable(bool enable);
    bool isEnabled() const;
    int id() const;
    QString name() const;
    QString description() const;

public slots:
    virtual void start() = 0;

protected:
    int m_id;
    QString m_name;
    QString m_description;
    QString m_enableParam;
    Logger *m_logger;

private:
    QString m_srcPort;
    QString m_dstPort;
};


class UnitTestInfo : public UnitTestBase
{
    Q_OBJECT
public:
    explicit UnitTestInfo(Logger *logger, QObject *parent = 0);

public slots:
    virtual void start();
};






class UnitTestFactory
{
public:
    static UnitTestBase *create(UnitTestBase::UnitID id, Logger *logger);
};


#endif // UNITTESTS_H
