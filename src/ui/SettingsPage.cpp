#include "ui/SettingsPage.h"

#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>

SettingsPage::SettingsPage(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);

    auto *form = new QFormLayout();
    m_deviceNameEdit = new QLineEdit(this);
    m_downloadDirEdit = new QLineEdit(this);
    m_udpPortSpin = new QSpinBox(this);
    m_tcpPortSpin = new QSpinBox(this);

    m_udpPortSpin->setRange(1, 65535);
    m_tcpPortSpin->setRange(1, 65535);

    form->addRow(QStringLiteral("设备名"), m_deviceNameEdit);
    form->addRow(QStringLiteral("下载目录"), m_downloadDirEdit);
    form->addRow(QStringLiteral("UDP 发现端口"), m_udpPortSpin);
    form->addRow(QStringLiteral("TCP 传输端口"), m_tcpPortSpin);

    layout->addLayout(form);
    layout->addStretch();
}

void SettingsPage::setConfig(const AppConfig &config)
{
    m_deviceNameEdit->setText(config.deviceName);
    m_downloadDirEdit->setText(config.downloadDir);
    m_udpPortSpin->setValue(config.udpPort);
    m_tcpPortSpin->setValue(config.tcpPort);
}
