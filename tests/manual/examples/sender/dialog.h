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
    void sendRequest();
    void readResponse();
    void processTimeout();

private:
    void setControlsEnabled(bool enable);
    void processError(const QString &error);

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

    QSerialPort m_serial;
    QByteArray m_response;
    QTimer m_timer;
};

#endif // DIALOG_H
