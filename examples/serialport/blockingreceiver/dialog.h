// Copyright (C) 2012 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef DIALOG_H
#define DIALOG_H

#include "receiverthread.h"

#include <QDialog>

QT_BEGIN_NAMESPACE

class QLabel;
class QLineEdit;
class QComboBox;
class QSpinBox;
class QPushButton;

QT_END_NAMESPACE

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = nullptr);

private slots:
    void startReceiver();
    void showRequest(const QString &s);
    void processError(const QString &s);
    void processTimeout(const QString &s);
    void activateRunButton();

private:
    int m_transactionCount = 0;
    QLabel *m_serialPortLabel = nullptr;
    QComboBox *m_serialPortComboBox = nullptr;
    QLabel *m_waitRequestLabel = nullptr;
    QSpinBox *m_waitRequestSpinBox = nullptr;
    QLabel *m_responseLabel = nullptr;
    QLineEdit *m_responseLineEdit = nullptr;
    QLabel *m_trafficLabel = nullptr;
    QLabel *m_statusLabel = nullptr;
    QPushButton *m_runButton = nullptr;

    ReceiverThread m_thread;
};

#endif // DIALOG_H
