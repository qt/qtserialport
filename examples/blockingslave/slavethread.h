/****************************************************************************
**
** Copyright (C) 2012 Denis Shienkov <scapig@yandex.ru>
** Contact: http://www.qt-project.org/
**
** This file is part of the QtSerialPort module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef SLAVETHREAD_H
#define SLAVETHREAD_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>

//! [0]
class SlaveThread : public QThread
{
    Q_OBJECT

public:
    SlaveThread(QObject *parent = 0);
    ~SlaveThread();

    void startSlave(const QString &portName, int waitTimeout, const QString &response);
    void run();

signals:
    void request(const QString &s);
    void error(const QString &s);
    void timeout(const QString &s);

private:
    QString portName;
    QString response;
    int waitTimeout;
    QMutex mutex;
    bool quit;
};
//! [0]

#endif // SLAVETHREAD_H
