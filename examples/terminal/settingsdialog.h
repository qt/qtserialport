#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QtAddOnSerialPort/serialport.h>

namespace Ui {
class SettingsDialog;
}

class QIntValidator;

QT_USE_NAMESPACE_SERIALPORT

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    struct Settings {
        QString name;
        qint32 rate;
        QString stringRate;
        SerialPort::DataBits dataBits;
        QString stringDataBits;
        SerialPort::Parity parity;
        QString stringParity;
        SerialPort::StopBits stopBits;
        QString stringStopBits;
        SerialPort::FlowControl flowControl;
        QString stringFlowControl;
    };

    explicit SettingsDialog(QWidget *parent = 0);
    ~SettingsDialog();

    Settings settings() const;

private slots:
    void showPortInfo(int idx);
    void apply();
    void checkCustomRatePolicy(int idx);

private:
    void fillPortsParameters();
    void fillPortsInfo();
    void updateSettings();

private:
    Ui::SettingsDialog *ui;
    Settings currentSettings;
    QIntValidator *intValidator;
};

#endif // SETTINGSDIALOG_H
