#include "ui/HomePage.h"

#include <QLabel>
#include <QVBoxLayout>

HomePage::HomePage(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(32, 32, 32, 32);
    layout->setSpacing(16);

    auto *title = new QLabel(QStringLiteral("AnDrop"), this);
    QFont titleFont = title->font();
    titleFont.setPointSize(24);
    titleFont.setBold(true);
    title->setFont(titleFont);

    m_deviceNameLabel = new QLabel(QStringLiteral("本机设备：AnDrop"), this);
    m_statusLabel = new QLabel(QStringLiteral("接收状态：每次询问"), this);
    m_onlineCountLabel = new QLabel(QStringLiteral("在线设备：0"), this);

    layout->addWidget(title);
    layout->addWidget(m_deviceNameLabel);
    layout->addWidget(m_statusLabel);
    layout->addWidget(m_onlineCountLabel);
    layout->addStretch();
}

void HomePage::setDeviceName(const QString &name)
{
    m_deviceNameLabel->setText(QStringLiteral("本机设备：%1").arg(name));
}

void HomePage::setOnlineDeviceCount(int count)
{
    m_onlineCountLabel->setText(QStringLiteral("在线设备：%1").arg(count));
}
