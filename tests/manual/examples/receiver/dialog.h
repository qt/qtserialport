// Copyright (C) 2012 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QSerialPort>
#include <QTimer>

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
    void readRequest();
    void processTimeout();
    void activateRunButton();

private:
    void processError(const QString &s);

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

    QSerialPort m_serial;
    QByteArray m_request;
    QTimer m_timer;
};

#endif // DIALOG_H
