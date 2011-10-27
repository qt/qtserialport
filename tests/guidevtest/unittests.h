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

class SerialPort;

class UnitTestBase : public QObject
{
    Q_OBJECT
signals:
    void finished();
    void error();

public:
    enum UnitID {
        InfoUnitId,
        SignalsUnitId,
        WaitForXUnitId,

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
    enum DirPorts { SrcPort, DstPort };
    int m_id;
    QString m_name;
    QString m_description;
    QString m_enableParam;
    Logger *m_logger;
    SerialPort *m_srcPort;
    SerialPort *m_dstPort;
    QString m_srcPortName;
    QString m_dstPortName;
};


class UnitTestInfo : public UnitTestBase
{
    Q_OBJECT
public:
    explicit UnitTestInfo(Logger *logger, QObject *parent = 0);

public slots:
    virtual void start();
};


class SerialPort;

class UnitTestSignals : public UnitTestBase
{
    Q_OBJECT
public:
    explicit UnitTestSignals(Logger *logger, QObject *parent = 0);

public slots:
    virtual void start();

private slots:
    void procSignalBytesWritten(qint64 bw);
    void procSignalReadyRead();
    void procSingleShot();
    void transaction();

private:
    enum {
        TransactionLimit = 5,
        TransactionMsecDelay = 1000,
        MinBytesToWrite = 1,
        StepBytesToWrite = 100

    };

    bool m_started;
    int m_transactionNum;
    qint64 m_bytesToWrite;
    qint64 m_bytesReallyWrited;
    int m_countSignalsBytesWritten;
    int m_countSignalsReadyRead;


    bool open(DirPorts dir);
    bool configure(DirPorts dir);
    void close(DirPorts dir);
};


class UnitTestWaitForX : public UnitTestBase
{
    Q_OBJECT
public:
    explicit UnitTestWaitForX(Logger *logger, QObject *parent = 0);

public slots:
    virtual void start();
};


class UnitTestFactory
{
public:
    static UnitTestBase *create(UnitTestBase::UnitID id, Logger *logger);
};


#endif // UNITTESTS_H
