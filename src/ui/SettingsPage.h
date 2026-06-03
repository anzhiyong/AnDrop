#pragma once

#include "core/AppConfig.h"

#include <QWidget>

class QLineEdit;
class QSpinBox;

class SettingsPage : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPage(QWidget *parent = nullptr);

    void setConfig(const AppConfig &config);

private:
    QLineEdit *m_deviceNameEdit = nullptr;
    QLineEdit *m_downloadDirEdit = nullptr;
    QSpinBox *m_udpPortSpin = nullptr;
    QSpinBox *m_tcpPortSpin = nullptr;
};
