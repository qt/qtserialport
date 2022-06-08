// Copyright (C) 2012 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef DIALOG_H
#define DIALOG_H

#include "senderthread.h"

#include <QDialog>

QT_BEGIN_NAMESPACE

class QLabel;
class QLineEdit;
class QSpinBox;
class QPushButton;
class QComboBox;

QT_END_NAMESPACE

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = nullptr);

private slots:
    void transaction();
    void showResponse(const QString &s);
    void processError(const QString &s);
    void processTimeout(const QString &s);

private:
    void setControlsEnabled(bool enable);

private:
    int m_transactionCount = 0;
    QLabel *m_serialPortLabel = nullptr;
    QComboBox *m_serialPortComboBox = nullptr;
    QLabel *m_waitResponseLabel = nullptr;
    QSpinBox *m_waitResponseSpinBox = nullptr;
    QLabel *m_requestLabel = nullptr;
    QLineEdit *m_requestLineEdit = nullptr;
    QLabel *m_trafficLabel = nullptr;
    QLabel *m_statusLabel = nullptr;
    QPushButton *m_runButton = nullptr;

    SenderThread m_thread;
};

#endif // DIALOG_H
