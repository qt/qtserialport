// Copyright (C) 2012 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef RECEIVERTHREAD_H
#define RECEIVERTHREAD_H

#include <QMutex>
#include <QThread>
#include <QWaitCondition>

//! [0]
class ReceiverThread : public QThread
{
    Q_OBJECT

public:
    explicit ReceiverThread(QObject *parent = nullptr);
    ~ReceiverThread();

    void startReceiver(const QString &portName, int waitTimeout, const QString &response);

signals:
    void request(const QString &s);
    void error(const QString &s);
    void timeout(const QString &s);

private:
    void run() override;

    QString m_portName;
    QString m_response;
    int m_waitTimeout = 0;
    QMutex m_mutex;
    bool m_quit = false;
};
//! [0]

#endif // RECEIVERTHREAD_H
