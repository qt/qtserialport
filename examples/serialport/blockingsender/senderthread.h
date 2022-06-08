// Copyright (C) 2012 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SENDERTHREAD_H
#define SENDERTHREAD_H

#include <QMutex>
#include <QThread>
#include <QWaitCondition>

//! [0]
class SenderThread : public QThread
{
    Q_OBJECT

public:
    explicit SenderThread(QObject *parent = nullptr);
    ~SenderThread();

    void transaction(const QString &portName, int waitTimeout, const QString &request);

signals:
    void response(const QString &s);
    void error(const QString &s);
    void timeout(const QString &s);

private:
    void run() override;

    QString m_portName;
    QString m_request;
    int m_waitTimeout = 0;
    QMutex m_mutex;
    QWaitCondition m_cond;
    bool m_quit = false;
};
//! [0]

#endif // SENDERTHREAD_H
