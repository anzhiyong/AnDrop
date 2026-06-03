#include "ui/DeviceListPage.h"

#include <QFileDialog>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

DeviceListPage::DeviceListPage(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);

    m_deviceList = new QListWidget(this);
    m_sendButton = new QPushButton(QStringLiteral("选择文件发送"), this);
    m_sendButton->setEnabled(false);

    layout->addWidget(m_deviceList, 1);
    layout->addWidget(m_sendButton);

    connect(m_deviceList, &QListWidget::currentRowChanged, this, [this](int row) {
        m_sendButton->setEnabled(row >= 0 && row < m_devices.size());
    });

    connect(m_sendButton, &QPushButton::clicked, this, [this]() {
        const int row = m_deviceList->currentRow();
        if (row < 0 || row >= m_devices.size()) {
            return;
        }

        const QStringList files = QFileDialog::getOpenFileNames(this, QStringLiteral("选择要发送的文件"));
        if (!files.isEmpty()) {
            emit sendFilesRequested(m_devices.at(row), files);
        }
    });
}

void DeviceListPage::setDevices(const QList<DeviceInfo> &devices)
{
    m_devices = devices;
    m_deviceList->clear();

    for (const DeviceInfo &device : m_devices) {
        m_deviceList->addItem(QStringLiteral("%1  %2  %3:%4")
            .arg(device.deviceName, device.platform, device.ipAddress)
            .arg(device.tcpPort));
    }

    m_sendButton->setEnabled(m_deviceList->currentRow() >= 0 && m_deviceList->currentRow() < m_devices.size());
}
